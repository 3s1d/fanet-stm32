#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stm32l4xx.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_spi.h"

#include "minmax.h"

#include "sx1272.h"

#ifdef SX1272_DO_FSK
#include <math.h>
#endif


bool sx1272_armed = false;
irq_callback sx1272_irq_cb = NULL;
SPI_HandleTypeDef *sx1272_spi = NULL;
float sx_dutycycle = 0.0f;
float sx_airtime = 0.0f;
sx_region_t sx1272_region = {.channel = 0, .dBm = 0, .bw = 0};
static bool sx_payloadCrc = false;

#ifdef SX1272_DO_FSK
#define SX_REG_BACKUP_POR			0
#define SX_REG_BACKUP_LORA			1
#define SX_REG_BACKUP_OFFSET		REG_BITRATE_MSB
uint8_t sx_reg_backup[2][111];
#endif

/*
 * private
 */

//note: disabling interrupt not required as fanet task will only  operate on this and an INT only generates a signal -> handled ion sync

static inline void sx_select()
{
	HAL_NVIC_DisableIRQ(SXDIO0_EXTI8_EXTI_IRQn);
	HAL_GPIO_WritePin(SXSEL_GPIO_Port, SXSEL_Pin, GPIO_PIN_RESET);
}

static inline void sx_unselect()
{
	HAL_GPIO_WritePin(SXSEL_GPIO_Port, SXSEL_Pin, GPIO_PIN_SET);
	HAL_NVIC_EnableIRQ(SXDIO0_EXTI8_EXTI_IRQn);
}

uint8_t sx_readRegister(uint8_t address)
{
	if(sx1272_spi == NULL)
		return 0;

	sx_select();

	/* bit 7 cleared to read in registers */
	address &= 0x7F;

	HAL_SPI_Transmit(sx1272_spi, &address, 1, 200);
	uint8_t value;
	HAL_SPI_Receive(sx1272_spi, &value, 1, 200);
	sx_unselect();

	return value;
}

void sx_writeRegister(uint8_t address, uint8_t data)
{
	sx_select();
	/* bit 7 set to write registers */
	address |= 0x80;

	uint8_t tx[2] = {address, data};
	HAL_SPI_Transmit(sx1272_spi, tx, 2, 200);
	sx_unselect();

#ifdef SX1272_DO_FSK
	/* store changes to default vector */
	address &= 0x7F;		//remote bit 7
	if(address >= SX_REG_BACKUP_OFFSET && (address - SX_REG_BACKUP_OFFSET) < sizeof(sx_reg_backup[SX_REG_BACKUP_LORA]))
		sx_reg_backup[SX_REG_BACKUP_LORA][address - SX_REG_BACKUP_OFFSET] = data;
#endif

}

void sx_writeRegister_nostore(uint8_t address, uint8_t data)
{
	sx_select();
	/* bit 7 set to write registers */
	address |= 0x80;

	uint8_t tx[2] = {address, data};
	HAL_SPI_Transmit(sx1272_spi, tx, 2, 200);
	sx_unselect();
}

uint8_t sx_readRegister_burst(uint8_t address, uint8_t *data, int length)
{
	if(sx1272_spi == NULL || data == NULL)
		return 0;

	sx_select();

	/* bit 7 cleared to read in registers */
	address &= 0x7F;

	HAL_SPI_Transmit(sx1272_spi, &address, 1, 200);
	HAL_StatusTypeDef ret = HAL_SPI_Receive(sx1272_spi, data, length, 200);
	sx_unselect();

	return (ret==HAL_OK)?length:0;
}

int sx_writeRegister_burst(uint8_t address, uint8_t *data, int length)
{
	if(sx1272_spi == NULL || data == NULL)
		return 0;

	sx_select();
	/* bit 7 set to write registers */
	address |= 0x80;

	HAL_SPI_Transmit(sx1272_spi, &address, 1, 200);
	HAL_StatusTypeDef ret = HAL_SPI_Transmit(sx1272_spi, data, length, 200);
	sx_unselect();

	return (ret==HAL_OK)?length:0;
}

bool sx_setOpMode(uint8_t mode)
{
	/* no mode change while tx */
	uint8_t opmode;
	uint16_t testLoops = 0;
	do
	{
		/* timeout */
		if(++testLoops > 5)
			HAL_Delay(1);
		if(testLoops > 300)
			return false;

		opmode = sx_readRegister(REG_OP_MODE);
	}while(SX_IN_X_TX_MODE(opmode) ||
			(SX_IN_RX_MODE(opmode) && (sx_readRegister(REG_MODEM_STAT) & 0x0B)));	//in tx mode or currently something receiving

	/* set mode */
	sx_writeRegister(REG_OP_MODE, mode);

	/* set rf analog end */
	switch (mode)
	{
	case LORA_RXSINGLE_MODE:
	case LORA_RXCONT_MODE:
	case LORA_CAD_MODE:
#ifdef SXTX_Pin
		HAL_GPIO_WritePin(SXTX_GPIO_Port, SXTX_Pin, GPIO_PIN_RESET);
#endif
#ifdef SXRX_Pin
		HAL_GPIO_WritePin(SXRX_GPIO_Port, SXRX_Pin, GPIO_PIN_SET);
#endif
	break;
	case LORA_TX_MODE:
	case GFSK_TX_MODE:
#ifdef SXRX_Pin
		HAL_GPIO_WritePin(SXRX_GPIO_Port, SXRX_Pin, GPIO_PIN_RESET);
#endif
#ifdef SXTX_Pin
		HAL_GPIO_WritePin(SXTX_GPIO_Port, SXTX_Pin, GPIO_PIN_SET);
#endif
	break;
	default:
#ifdef SXTX_Pin
		HAL_GPIO_WritePin(SXTX_GPIO_Port, SXTX_Pin, GPIO_PIN_RESET);
#endif
#ifdef SXRX_Pin
		HAL_GPIO_WritePin(SXRX_GPIO_Port, SXRX_Pin, GPIO_PIN_RESET);
#endif
		break;
	}

	/* wait for frequency synthesis, 70ms timeout */
	//note: LORA_CAD_MODE may instantaneously end in LORA_STANDBY_MODE
	opmode = 0;
	const uint8_t mask = (mode == LORA_CAD_MODE) ? 0xf9 : 0xff;
	for(int i=0; i<70 && opmode != (mode & mask); i++)
	{
		HAL_Delay(1);
		opmode = sx_readRegister(REG_OP_MODE) & mask;

		/* resend mode */
		if(i && i%20 == 0)
			sx_writeRegister(REG_OP_MODE, mode);
	}

#ifdef DEBUG
//	if((mode&mask) != opmode)
//		debug_printf("mode change failed. is%02X set%02X\n", opmode, mode);
#endif
	return (mode&mask) == opmode;
}

uint8_t sx_getOpMode(void)
{
	return sx_readRegister(REG_OP_MODE);
}

int sx_writeFifo(uint8_t addr, uint8_t *data, int length)
{
	/* select location */
	sx_writeRegister(REG_FIFO_ADDR_PTR, addr);

	/* write data */
	return sx_writeRegister_burst(REG_FIFO, data, length);
}

int sx_readFifo(uint8_t addr, uint8_t *data, int length)
{
	/* select location */
	sx_writeRegister(REG_FIFO_ADDR_PTR, addr);

	/* read data */
	return sx_readRegister_burst(REG_FIFO, data, length);
}

void sx_setDio0Irq(int mode)
{
	uint8_t map1 = sx_readRegister(REG_DIO_MAPPING1) & 0x3F;
	sx_writeRegister(REG_DIO_MAPPING1, map1 | mode);
}

float sx_expectedAirTime_ms(void)
{
#ifdef SX1272_DO_FSK
	uint8_t mode = sx_getOpMode();
	if(SX_IN_FSK_MODE(mode))
	{
		/* FSK */
		//note: fixed length, no address, ignoring CRC (as it's done in software here)
		int nbytes = ((sx_readRegister(REG_PREAMBLE_MSB_FSK) << 8) | sx_readRegister(REG_PREAMBLE_LSB_FSK)) +	//preamble
				(sx_readRegister(REG_SYNC_CONFIG)&SYNCSIZE_MASK_FSK) + 1 +				//syncword
				sx_readRegister(REG_PAYLOAD_LENGTH_FSK) * (sx_readRegister(REG_PACKET_CONFIG1)&0x20?2:1);//payload, assuming <256
		//int bitrate = (sx_readRegister(REG_BITRATE_MSB)<<8) | sx_readRegister(REG_BITRATE_LSB);
		int bitrate = (0x01<<8) | 0x40;			//note: hard coded
		if(bitrate <= 0)
			bitrate = 100;
		bitrate = 32e6 / bitrate;

		float air_time = (8.0f * nbytes) / bitrate * 1e3;
		return air_time;
	}
#endif
	/* LORA */
        float bw = 0.0f;
#ifdef SX1272_DO_FSK
        uint8_t cfg1 = sx_reg_backup[SX_REG_BACKUP_LORA][REG_MODEM_CONFIG1 - SX_REG_BACKUP_OFFSET];
#else
        uint8_t cfg1 = sx_readRegister(REG_MODEM_CONFIG1);
#endif
        uint8_t bw_reg = cfg1 & 0xC0;
        switch( bw_reg )
        {
        case 0:		// 125 kHz
            bw = 125000.0f;
            break;
        case 0x40: 	// 250 kHz
            bw = 250000.0f;
            break;
        case 0x80: 	// 500 kHz
            bw = 500000.0f;
            break;
        }

        // Symbol rate : time for one symbol (secs)
#ifdef SX1272_DO_FSK
        uint8_t sf_reg = sx_reg_backup[SX_REG_BACKUP_LORA][REG_MODEM_CONFIG2 - SX_REG_BACKUP_OFFSET] >> 4;
#else
        uint8_t sf_reg = sx_readRegister(REG_MODEM_CONFIG2)>>4;
#endif
        //float sf = sf_reg<6 ? 6.0f : sf_reg;
        float rs = bw / ( 1 << sf_reg );
        float ts = 1 / rs;
#ifdef SX1272_DO_FSK
        /* time of preamble */
        //note: assuming preamble < 256
        float tPreamble = ( sx_reg_backup[SX_REG_BACKUP_LORA][REG_PREAMBLE_LSB_LORA - SX_REG_BACKUP_OFFSET] + 4.25f ) * ts;
#else
        // time of preamble
        float tPreamble = ( sx_readRegister(REG_PREAMBLE_LSB_LORA) + 4.25f ) * ts;		//note: assuming preamble < 256
#endif
        /* Symbol length of payload and time */
        //note: assuming CRC on -> 16, fixed length
        int pktlen = sx_readRegister(REG_PAYLOAD_LENGTH_LORA);

        int coderate = (cfg1 >> 3) & 0x7;
        float tmp = ceil( (8 * pktlen - 4 * sf_reg + 28 + 16 - 20) / (float)( 4 * ( sf_reg ) ) ) * ( coderate + 4 );
        float nPayload = 8 + ( ( tmp > 0 ) ? tmp : 0 );
        float tPayload = nPayload * ts;
        // Time on air
        float tOnAir = (tPreamble + tPayload) * 1000.0f;

        return tOnAir;
}

bool sx_receiveStart(void)
{
	if(sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	/* prepare receiving */
	sx_writeRegister(REG_FIFO_RX_BASE_ADDR, 0x00);
	sx_writeRegister(REG_PAYLOAD_LENGTH_LORA, 0xFF);		//is this evaluated in explicit mode?

	/* clear any rx done flag */
	sx_writeRegister(REG_IRQ_FLAGS, IRQ_RX_DONE);

	/* switch irq behavior to rx_done and enter rx cont mode */
	sx_setDio0Irq(DIO0_RX_DONE_LORA);
	return sx_setOpMode(LORA_RXCONT_MODE);
}

bool sx_toLoRa(void)
{
	/* FSK mode -> enter normal mode again */
	//note: we currently only support TX on FSK
	sx_setOpMode(GFSK_SLEEP_MODE);
	if(sx_setOpMode(LORA_SLEEP_MODE) == false)
		return false;

	/* drop all interrupts */
	sx_reg_backup[SX_REG_BACKUP_LORA][REG_IRQ_FLAGS - SX_REG_BACKUP_OFFSET] = 0xFF;

	if(sx_writeRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_LORA], sizeof(sx_reg_backup[SX_REG_BACKUP_LORA])) <
			sizeof(sx_reg_backup[SX_REG_BACKUP_LORA]))
		sx_writeRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_LORA], sizeof(sx_reg_backup[SX_REG_BACKUP_LORA]));

	/* switch irq behavior back and re-enter rx mode */
	if(sx1272_armed)
		return sx_receiveStart();
	return true;
}


int sx_channel_free4tx(bool doCAD)
{
	uint8_t mode = sx_getOpMode();
	if(SX_IN_FSK_MODE(mode))
		return TX_FSK_ONGOING;
	mode &= LORA_MODE_MASK;

	/* are we transmitting anyway? */
	if(mode == LORA_TX_MODE || mode == LORA_FSTX_MODE)
		return TX_TX_ONGOING;

	/* in case of receiving, is it ongoing? */
	for(uint16_t i=0; i<4 && (mode == LORA_RXCONT_MODE || mode == LORA_RXSINGLE_MODE); i++)
	{
		if(sx_readRegister(REG_MODEM_STAT) & 0x0B)
			return TX_RX_ONGOING;
		HAL_Delay(1);
	}

	/* CAD not required */
	if(doCAD == false)
		return TX_OK;

	/*
	 * CAD
	 */

	if(sx_setOpMode(LORA_STANDBY_MODE) == false)
		return TX_ERROR;
	sx_writeRegister(REG_IRQ_FLAGS, IRQ_CAD_DONE | IRQ_CAD_DETECTED);	/* clearing flags */
	sx_setOpMode(LORA_CAD_MODE);

	/* wait for CAD completion */
	uint8_t iflags;
	for(uint16_t i = 0; i<10 && ((iflags=sx_readRegister(REG_IRQ_FLAGS)) & IRQ_CAD_DONE) == 0; i++)
		HAL_Delay(1);

	if(iflags & IRQ_CAD_DETECTED)
	{
		/* re-establish old mode */
		if(mode == LORA_RXCONT_MODE || mode == LORA_RXSINGLE_MODE || mode == LORA_SLEEP_MODE)
			sx_setOpMode(mode);

		return TX_RX_ONGOING;
	}

	return TX_OK;
}

/*
 * public
 */

bool sx1272_init(SPI_HandleTypeDef *spi)
{
	sx1272_spi = spi;

	/* Pins */
	//note: are assumed to be configured
	sx_unselect();

	/* Configure SPI */
	//note: spi configured in spi.c

	/* Reset pulse for LoRa module initialization */
	HAL_GPIO_WritePin(SXRESET_GPIO_Port, SXRESET_Pin, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(SXRESET_GPIO_Port, SXRESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(6);

	/* set Lora mode, this is only possible during sleep -> do it twice */
	sx_setOpMode(LORA_SLEEP_MODE);
	sx_setOpMode(LORA_SLEEP_MODE);

#ifdef SX1272_DO_FSK
	/* get POR registers */
	//note: required for FSK transition, storing is faster than performing a reset
	sx_readRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_POR], sizeof(sx_reg_backup[SX_REG_BACKUP_POR]));
	sx_readRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_LORA], sizeof(sx_reg_backup[SX_REG_BACKUP_LORA]));

	/* check data integrety */
	if(memcmp(sx_reg_backup[SX_REG_BACKUP_POR], sx_reg_backup[SX_REG_BACKUP_LORA], sizeof(sx_reg_backup[SX_REG_BACKUP_POR])) != 0)
	{
		HAL_Delay(2);
		sx_readRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_POR], sizeof(sx_reg_backup[SX_REG_BACKUP_POR]));
		sx_readRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_LORA], sizeof(sx_reg_backup[SX_REG_BACKUP_LORA]));
	}
#endif

	/* set standby mode */
	bool mode = sx_setOpMode(LORA_STANDBY_MODE);

	/* some more tweaks here */
	/* disable LowPnTxPllOff -> low phase noise PLL in transmit mode, however more current */
	sx_writeRegister(REG_PA_RAMP, 0x09);
	/* disable over current detection */
	sx_writeRegister(REG_OCP, 0x1B);

	return mode;
}

bool sx1272_setBandwidth(uint8_t bw)
{
	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE &&sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	uint8_t config1 = sx_readRegister(REG_MODEM_CONFIG1);
	config1 = (bw&BW_MASK) | (config1&(~BW_MASK));
	sx_writeRegister(REG_MODEM_CONFIG1, config1);

	/* adjust low data rate for high spread factor */
	if(bw == BW_125)
	{
		uint8_t sf = sx1272_getSpreadingFactor();
		if(sf == SF_11 || sf == SF_12)
			sx1272_setLowDataRateOptimize(true);
	}

	/* restore state */
	return opmode == LORA_STANDBY_MODE || sx_setOpMode(opmode);
}

uint8_t sx1272_getBandwidth(void)
{
	uint8_t bw = sx_readRegister(REG_MODEM_CONFIG1) & BW_MASK;

	return bw;
}

void sx1272_setLowDataRateOptimize(bool ldro)
{
	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE)
		sx_setOpMode(LORA_STANDBY_MODE);

	uint8_t config1 = sx_readRegister(REG_MODEM_CONFIG1);
	config1 = (config1&0xFE) | !!ldro;
	sx_writeRegister(REG_MODEM_CONFIG1, config1);

	/* restore state */
	if(opmode != LORA_STANDBY_MODE)
		sx_setOpMode(opmode);
}

bool sx1272_setPayloadCrc(bool crc)
{
	sx_payloadCrc = crc;

	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE && sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	uint8_t config1 = sx_readRegister(REG_MODEM_CONFIG1);
	config1 = (config1&0xFD) | (!!crc)<<1;
	sx_writeRegister(REG_MODEM_CONFIG1, config1);

	/* restore state */
	return opmode == LORA_STANDBY_MODE || sx_setOpMode(opmode);
}

bool sx1272_setSpreadingFactor(uint8_t spr)
{
	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE && sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	uint8_t config2 = sx_readRegister(REG_MODEM_CONFIG2);
	config2 = (spr&SF_MASK) | (config2&~SF_MASK);

	/* LowDataRateOptimize (Mandatory with SF_11 or SF_12 if BW_125) */
	if((spr == SF_11 || spr == SF_12) && sx1272_getBandwidth() == BW_125)
		sx1272_setLowDataRateOptimize(true);

	sx_writeRegister(REG_MODEM_CONFIG2, config2);

	/* Check if it is necessary to set special settings for SF=6 */
	if (spr == SF_6)
	{
		// Mandatory headerOFF with SF = 6 (Implicit mode)
		sx1272_setExplicitHeader(false);
		sx_writeRegister(REG_DETECT_OPTIMIZE, 0x05);
		sx_writeRegister(REG_DETECTION_THRESHOLD, 0x0C);
	}
	else
	{
		sx_writeRegister(REG_DETECT_OPTIMIZE, 0x03);
		sx_writeRegister(REG_DETECTION_THRESHOLD, 0x0A);
	}

	/* restore state */
	return opmode == LORA_STANDBY_MODE || sx_setOpMode(opmode);
}

uint8_t sx1272_getSpreadingFactor(void)
{
	uint8_t sf = sx_readRegister(REG_MODEM_CONFIG2) & SF_MASK;

	return sf;
}

bool sx1272_setCodingRate(uint8_t cr)
{
	/* store state */
	uint8_t opmode = sx_getOpMode();
	if(opmode == LORA_TX_MODE)
		return false;

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE && sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	uint8_t config1 = sx_readRegister(REG_MODEM_CONFIG1);
	config1 = (cr&CR_MASK) | (config1&~CR_MASK);
	sx_writeRegister(REG_MODEM_CONFIG1, config1);

	/* restore state */
	return opmode == LORA_STANDBY_MODE || sx_setOpMode(opmode);
}

uint8_t sx1272_getCodingRate(void)
{
	uint8_t cr = sx_readRegister(REG_MODEM_CONFIG1) & CR_MASK;

	return cr;
}

bool sx1272_setChannel(uint32_t ch)
{
	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE && sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	uint8_t freq3 = (ch >> 16) & 0x0FF;		// frequency channel MSB
	uint8_t freq2 = (ch >> 8) & 0xFF;		// frequency channel MID
	uint8_t freq1 = ch & 0xFF;			// frequency channel LSB

	sx_writeRegister(REG_FRF_MSB, freq3);
	sx_writeRegister(REG_FRF_MID, freq2);
	sx_writeRegister(REG_FRF_LSB, freq1);

	/* restore state */
	return opmode == LORA_STANDBY_MODE || sx_setOpMode(opmode);
}

uint32_t sx1272_getChannel(void)
{
	uint8_t freq3 = sx_readRegister(REG_FRF_MSB);	// frequency channel MSB
	uint8_t freq2 = sx_readRegister(REG_FRF_MID);	// frequency channel MID
	uint8_t freq1 = sx_readRegister(REG_FRF_LSB);	// frequency channel LSB
	uint32_t ch = ((uint32_t) freq3 << 16) + ((uint32_t) freq2 << 8) + (uint32_t) freq1;

	return ch;
}

bool sx1272_setExplicitHeader(bool exhdr)
{
	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE && sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	/* bit2: 0->explicit 1->implicit */
	uint8_t config1 = sx_readRegister(REG_MODEM_CONFIG1);
	config1 = (config1&0xFB) | (!exhdr)<<2;
	sx_writeRegister(REG_MODEM_CONFIG1, config1);

	/* restore state */
	return opmode == LORA_STANDBY_MODE || sx_setOpMode(opmode);
}

void sx1272_setLnaGain(uint8_t gain, bool lnaboost)
{
	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE)
		sx_setOpMode(LORA_STANDBY_MODE);

	sx_writeRegister(REG_LNA, (gain<<LNAGAIN_OFFSET) | (lnaboost?0x3:0x0));

	/* restore state */
	if(opmode != LORA_STANDBY_MODE)
		sx_setOpMode(opmode);
}

//note: we currently only support PA_BOOST
bool sx1272_setPower(int pwr)
{
	/* clamp pwr requirement */
	if(pwr < 2)
		pwr = 2;
	if(pwr > 20)
		pwr = 20;

	/* store state */
	uint8_t opmode = sx_getOpMode();

	/* set appropriate mode */
	if(opmode != LORA_STANDBY_MODE && sx_setOpMode(LORA_STANDBY_MODE) == false)
		return false;

	if(pwr >= 18)
	{
		/* high power mode */
		sx_writeRegister(REG_PA_DAC, PA_DAC_HIGH_PWR);
		pwr -= 5;
	}
	else
	{
		/* normal mode */
		sx_writeRegister(REG_PA_DAC, PA_DAC_DEFAULT_PWR);
		pwr -= 2;
	}

	/* PA output pin to PA_BOOST */
	sx_writeRegister(REG_PA_CONFIG, 0x80 | pwr);

	/* restore state */
	return opmode == LORA_STANDBY_MODE || sx_setOpMode(opmode);
}

bool sx1272_setArmed(bool mode)
{
	if(mode == sx1272_armed)
		return true;

	/* transmit packet in buffer */
	uint8_t opMode = sx_getOpMode();
	while(SX_IN_X_TX_MODE(opMode))
	{
		HAL_Delay(1);
		opMode = sx_getOpMode();
	}

	if(mode)
	{
		/* enable rx */
		sx1272_armed = sx_receiveStart();
		if(sx1272_armed == false)
			sx1272_armed = sx_receiveStart();
	}
	else
	{
		/* enter power save */
		sx1272_armed = !sx_setOpMode(LORA_SLEEP_MODE);
		if(sx1272_armed == true)
			sx1272_armed = !sx_setOpMode(LORA_SLEEP_MODE);

	}

	return sx1272_armed == mode;
}

bool sx1272_isArmed(void)
{
	return sx1272_armed;
}

bool sx1272_reset(bool hard)
{
	/* perform hardware reset */
	if(hard)
	{
		HAL_GPIO_WritePin(SXRESET_GPIO_Port, SXRESET_Pin, GPIO_PIN_SET);
		HAL_Delay(1);
		HAL_GPIO_WritePin(SXRESET_GPIO_Port, SXRESET_Pin, GPIO_PIN_RESET);
		HAL_Delay(7);
	}

	/* enter sleep mode soft way */
	sx_setOpMode(GFSK_SLEEP_MODE);
	sx_setOpMode(LORA_SLEEP_MODE);

	bool ret = sx_writeRegister_burst(REG_BITRATE_MSB, sx_reg_backup[SX_REG_BACKUP_LORA], sizeof(sx_reg_backup[SX_REG_BACKUP_LORA]));

	/* switch irq behavior back and re-enter rx mode */
	if(sx1272_armed && ret)
		ret &= sx_receiveStart();

	return ret;
}

//note: do not use hardware based interrupts here
void sx1272_irq(void)
{
#ifdef SX1272_DO_FSK
	uint8_t opMode = sx_getOpMode();
	if(SX_IN_FSK_MODE(opMode))
	{
		sx_toLoRa();
		return;
	}
#endif

	/* enter sleep mode */
	//note: does not destroy MSB (LORA <-> FSK)
	sx_setOpMode(LORA_STANDBY_MODE);

	if(sx1272_irq_cb == NULL)
	{
		sx_writeRegister(REG_IRQ_FLAGS, 0xFF);
		return;
	}

	uint8_t what = sx_readRegister(REG_IRQ_FLAGS);

	/* Tx done */
	//if(what & IRQ_TX_DONE)
	//{
		//nothing to do...
	//}

	/* Rx done, CRC is valid and CRC was transmitted */
	if((what & IRQ_RX_DONE) && !(what & IRQ_PAYLOAD_CRC_ERROR) && (sx_readRegister(REG_HOP_CHANNEL) & CRC_ON_PAYLOAD))
	{
		/* retrieve data */
		const int received = sx_readRegister(REG_RX_NB_BYTES);

		/* callback function which is supposed to read the fifo */
		if(received > 0 && sx1272_irq_cb != NULL)
			sx1272_irq_cb(received);
	}

	/* clear all flag */
	sx_writeRegister(REG_IRQ_FLAGS, 0xFF);

	/* switch irq behavior back and re-enter rx mode */
	if(sx1272_armed)
		sx_receiveStart();
}


void sx1272_setIrqReceiver(irq_callback cb)
{
	sx_setOpMode(LORA_STANDBY_MODE);
	sx_setDio0Irq(DIO0_RX_DONE_LORA);

	/* clear all flag */
	sx_writeRegister(REG_IRQ_FLAGS, 0xFF);

	sx1272_irq_cb = cb;
	HAL_NVIC_EnableIRQ(SXDIO0_EXTI8_EXTI_IRQn);
}

void sx1272_clrIrqReceiver(void)
{
	sx_setOpMode(LORA_STANDBY_MODE);

	HAL_NVIC_DisableIRQ(SXDIO0_EXTI8_EXTI_IRQn);
	sx1272_irq_cb = NULL;
}

bool sx1272_setRegion(sx_region_t region)
{
	bool success = sx1272_setChannel(region.channel);
	if(success)
		success = sx1272_setPower(region.dBm);
	if(success)
		success = sx1272_setBandwidth(region.bw);

	if(success)
		sx1272_region = region;

	return success;
}

const sx_region_t *getRegion(void)
{
	return &sx1272_region;
}

void sx1272_setSyncWord(uint8_t sw)
{
	sx_writeRegister(REG_SYNCWORD_LORA, sw);
}

int sx1272_getRssi(void)
{
	/* get values */
	const int pktsnr = (int8_t) sx_readRegister(REG_PKT_SNR_VALUE);
	int rssi = -139 + sx_readRegister(REG_PKT_RSSI_VALUE);
	if(pktsnr < 0)
		rssi += ((pktsnr-2)/4);			//note: correct rounding for negative numbers

	return rssi;
}

/***********************************/
int sx1272_sendFrame(uint8_t *data, int length, uint8_t cr)
{
	/* channel accessible? */
	int state = sx_channel_free4tx(true);
	if(state != TX_OK)
		return state;

	/*
	 * Prepare TX
	 */
	sx_setOpMode(LORA_STANDBY_MODE);

	//todo: check fifo is empty, no rx data..

	/* adjust coding rate */
	//note: crc payload flag seems to get lost due to unknown issues!?!? resetting here just to be sure... fixme
	sx1272_setCodingRate(cr);
	sx1272_setPayloadCrc(sx_payloadCrc);

	/* upload frame */
	sx_writeFifo(0x00, data, length);
	sx_writeRegister(REG_FIFO_TX_BASE_ADDR, 0x00);
	sx_writeRegister(REG_PAYLOAD_LENGTH_LORA, length);

	/* prepare irq */
	if(sx1272_irq_cb)
	{
		/* clear flag */
		sx_writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE);
		sx_setDio0Irq(DIO0_TX_DONE_LORA);
	}

	/* update air time */
	sx_airtime += sx_expectedAirTime_ms();

	/* tx */
	sx_setOpMode(LORA_TX_MODE);

	/* ensure we enter tx mode before leaving */
	HAL_Delay(2);

	/* bypass waiting */
	if(sx1272_irq_cb)
		return TX_OK;

	for(int i=0; i<100 && sx_getOpMode() == LORA_TX_MODE; i++)
		HAL_Delay(5);

	return TX_OK;
}

int sx1272_receiveFrame(uint8_t *data, int max_length)
{
	sx_setOpMode(LORA_STANDBY_MODE);

	/* prepare receiving */
	sx_writeRegister(REG_FIFO_RX_BASE_ADDR, 0x00);
	sx_writeRegister(REG_PAYLOAD_LENGTH_LORA, max_length);
	//writeRegister(REG_SYMB_TIMEOUT_LSB, 0xFF);	//test max payload

	sx_setOpMode(LORA_RXCONT_MODE);

	/* wait for rx or timeout */
	for(int i=0; i<400 && !(sx_readRegister(REG_IRQ_FLAGS) & 0x40); i++)
		HAL_Delay(5);

	/* stop rx mode and retrieve data */
	sx_setOpMode(LORA_STANDBY_MODE);
	const int received = sx_readRegister(REG_RX_NB_BYTES);
	const int rxstartaddr = sx_readRegister(REG_RX_NB_BYTES);
	sx_readFifo(rxstartaddr, data, min(received, max_length));

	/* clear the rxDone flag */
	sx_writeRegister(REG_IRQ_FLAGS, IRQ_RX_DONE);

	return received;
}

int sx1272_getFrame(uint8_t *data, int max_length)
{
	const int received = sx_readRegister(REG_RX_NB_BYTES);
	const int rxstartaddr = sx_readRegister(REG_FIFO_RX_CURRENT_ADDR);
	sx_readFifo(rxstartaddr, data, min(received, max_length));

	return min(received, max_length);
}

float sx1272_get_airlimit(void)
{
	static uint32_t last = 0;
	uint32_t current = HAL_GetTick();
	uint32_t dt = current - last;
	last = current;

	/* reduce airtime by 1% */
	sx_airtime -= dt*0.01f;
	if(sx_airtime < 0.0f)
		sx_airtime = 0.0f;

	/* air time over 3min average -> 1800ms air time allowed in 1% terms */
	return sx_airtime / 1800.0f;
}

#ifdef SX1272_DO_FSK

/*
 *
 * FSK extension
 *
 */

//note: this method is highly optimized to switch between GFSK and LORA
int sx1272_sendFrame_FSK(sx_fsk_conf_t *conf, uint8_t *data, int num_data)
{
	if(conf == NULL || conf->num_syncword<=0)
		return TX_ERROR;

	/* channel accessible? */
	int state = sx_channel_free4tx(false);
	if(state != TX_OK)
		return state;

	/*
	 * Store register content, switch to FSK, and prepare TX
	 */
	if(sx_setOpMode(LORA_SLEEP_MODE) == false)
		return TX_ERROR;
	sx_writeRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_POR], sizeof(sx_reg_backup[SX_REG_BACKUP_POR]));
	sx_setOpMode(GFSK_SLEEP_MODE);

	/* bitrate 100kbps -> 320 */
	sx_writeRegister_nostore(REG_BITRATE_MSB, 0x01);
	sx_writeRegister_nostore(REG_BITRATE_LSB, 0x40);

	/* freq */
	int frf = round(((double)conf->frep) * 1638.4116);			//correct value: 1638.4 (don't use float)
	sx_writeRegister_nostore(REG_FRF_MSB, (frf>>16) & 0xFF);
	sx_writeRegister_nostore(REG_FRF_MID, (frf>>8) & 0xFF);
	sx_writeRegister_nostore(REG_FRF_LSB, frf & 0xFF);

	/* fdev 50khz -> 819.2(0x333), 100kHz -> 1638.4(0x666) */
	sx_writeRegister_nostore(REG_FDEV_MSB, 0x03);
	sx_writeRegister_nostore(REG_FDEV_LSB, 0x33);

	/* PA output pin to PA_BOOST, power to default value */
	int pwr_dBm = sx1272_region.dBm;
	if(pwr_dBm >= 18)
	{
		/* high power mode */
		sx_writeRegister_nostore(REG_PA_DAC, PA_DAC_HIGH_PWR);
		pwr_dBm -= 5;
	}
	else
	{
		/* normal mode */
		sx_writeRegister_nostore(REG_PA_DAC, PA_DAC_DEFAULT_PWR);
		pwr_dBm -= 2;
	}
	sx_writeRegister_nostore(REG_PA_CONFIG, 0x80 |  max(0, pwr_dBm));

	/* disable LowPnTxPllOff -> low phase noise PLL in transmit mode, however more current */
	sx_writeRegister_nostore(REG_PA_RAMP, 0x09);
	/* disable over current detection */
	sx_writeRegister_nostore(REG_OCP, 0x1B);

	/* set num_syncword, preamble polarity to 0x55 */
	sx_writeRegister_nostore(REG_SYNC_CONFIG, PREMBLEPOLARITY_FSK | SYNCON_FSK | ((conf->num_syncword-1) & SYNCSIZE_MASK_FSK));

	/* set syncword */
	sx_writeRegister_burst(REG_SYNC_VALUE1, conf->syncword, min(8,conf->num_syncword));

	/* manchester coding, no CRC */
	sx_writeRegister_nostore(REG_PACKET_CONFIG1, 0x20);

	/* packet mode */
	sx_writeRegister_nostore(REG_PACKET_CONFIG2, 0x40);

	/* upload data */
	sx_writeRegister_nostore(REG_PAYLOAD_LENGTH_FSK, num_data&0xFF);	//we silently assume max 64bytes as data due to hardware fifo limits
	sx_writeRegister_burst(REG_FIFO, data, num_data);

	/* prepare irq */
	if(sx1272_irq_cb)
		sx_setDio0Irq(DIO0_PACKET_SENT_FSK);
	else
		sx_setDio0Irq(DIO0_NONE_FSK);

	/* update air time */
	sx_airtime += sx_expectedAirTime_ms();

	/* start TX */
	//note: seq: tx on start, from transmit -> lowpower, lowpower = seq_off (w/ init mode), start
#ifdef SXRX_Pin
		HAL_GPIO_WritePin(SXRX_GPIO_Port, SXRX_Pin, GPIO_PIN_RESET);
#endif
#ifdef SXTX_Pin
		HAL_GPIO_WritePin(SXTX_GPIO_Port, SXTX_Pin, GPIO_PIN_SET);
#endif
	sx_writeRegister_nostore(REG_SEQ_CONFIG1, 0x80 | 0x10);

	/* ensure we enter tx mode before leaving */
	//note: transition to GFSK_TX_MODE is not immediately!
	HAL_Delay(2);

	/* bypass waiting */
	if(sx1272_irq_cb)
		return TX_OK;

	/* wait for completion */
	HAL_Delay(2);
	for(int i=0; i<100 && (sx_getOpMode()&0x07) == 0x03; i++)
		HAL_Delay(2);

	/* reestablish state */
	sx_setOpMode(GFSK_SLEEP_MODE);
	sx_setOpMode(LORA_SLEEP_MODE);
	sx_writeRegister_burst(SX_REG_BACKUP_OFFSET, sx_reg_backup[SX_REG_BACKUP_LORA], sizeof(sx_reg_backup[SX_REG_BACKUP_LORA]));

	return TX_OK;
}

#endif
