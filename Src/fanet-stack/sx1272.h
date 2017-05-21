#ifndef SX1272_h
#define SX1272_h

#include <stdint.h>
#include <inttypes.h>

#define SX1272_debug_mode 				0

//! MACROS //
//#define bitRead(value, bit) (((value) >> (bit)) & 0x01)  // read a bit
//#define bitSet(value, bit) ((value) |= (1UL << (bit)))    // set bit to '1'
//#define bitClear(value, bit) ((value) &= ~(1UL << (bit))) // set bit to '0'

//! REGISTERS //

#define        REG_FIFO        	                        0x00
#define        REG_OP_MODE                              0x01
#define        REG_BITRATE_MSB    			0x02
#define        REG_BITRATE_LSB    			0x03
#define        REG_FDEV_MSB   				0x04
#define        REG_FDEV_LSB    				0x05
#define        REG_FRF_MSB    				0x06
#define        REG_FRF_MID    				0x07
#define        REG_FRF_LSB    				0x08
#define        REG_PA_CONFIG    			0x09
#define        REG_PA_RAMP    				0x0A
#define        REG_OCP    				0x0B
#define        REG_LNA    				0x0C
#define        REG_RX_CONFIG    			0x0D
#define        REG_FIFO_ADDR_PTR  			0x0D
#define        REG_RSSI_CONFIG   			0x0E
#define        REG_FIFO_TX_BASE_ADDR 			0x0E
#define        REG_RSSI_COLLISION    			0x0F
#define        REG_FIFO_RX_BASE_ADDR                    0x0F
#define        REG_RSSI_THRESH    			0x10
#define        REG_FIFO_RX_CURRENT_ADDR                 0x10
#define        REG_RSSI_VALUE_FSK                       0x11
#define        REG_IRQ_FLAGS_MASK			0x11
#define        REG_RX_BW		    		0x12
#define        REG_IRQ_FLAGS	    			0x12
#define        REG_AFC_BW		    		0x13
#define        REG_RX_NB_BYTES	    			0x13
#define        REG_OOK_PEAK	    			0x14
#define        REG_RX_HEADER_CNT_VALUE_MSB 		0x14
#define        REG_OOK_FIX	    			0x15
#define        REG_RX_HEADER_CNT_VALUE_LSB  		0x15
#define        REG_OOK_AVG	 			0x16
#define        REG_RX_PACKET_CNT_VALUE_MSB  		0x16
#define        REG_RX_PACKET_CNT_VALUE_LSB  		0x17
#define        REG_MODEM_STAT	  			0x18
#define        REG_PKT_SNR_VALUE	  		0x19
#define        REG_AFC_FEI	  			0x1A
#define        REG_PKT_RSSI_VALUE	  		0x1A
#define        REG_AFC_MSB	  			0x1B
#define        REG_RSSI_VALUE_LORA	  		0x1B
#define        REG_AFC_LSB	  			0x1C
#define        REG_HOP_CHANNEL	  			0x1C
#define        REG_FEI_MSB	  			0x1D
#define        REG_MODEM_CONFIG1	 		0x1D
#define        REG_FEI_LSB	  			0x1E
#define        REG_MODEM_CONFIG2	  		0x1E
#define        REG_PREAMBLE_DETECT  			0x1F
#define        REG_SYMB_TIMEOUT_LSB  			0x1F
#define        REG_RX_TIMEOUT1	  			0x20
#define        REG_PREAMBLE_MSB_LORA  			0x20
#define        REG_RX_TIMEOUT2	  			0x21
#define        REG_PREAMBLE_LSB_LORA  			0x21
#define        REG_RX_TIMEOUT3	 			0x22
#define        REG_PAYLOAD_LENGTH_LORA			0x22
#define        REG_RX_DELAY	 			0x23
#define        REG_MAX_PAYLOAD_LENGTH 			0x23
#define        REG_OSC		 			0x24
#define        REG_HOP_PERIOD	  			0x24
#define        REG_PREAMBLE_MSB_FSK 			0x25
#define        REG_FIFO_RX_BYTE_ADDR 			0x25
#define        REG_PREAMBLE_LSB_FSK 			0x26
#define        REG_SYNC_CONFIG	  			0x27
#define        REG_SYNC_VALUE1	 			0x28
#define        REG_SYNC_VALUE2	  			0x29
#define        REG_SYNC_VALUE3	  			0x2A
#define        REG_SYNC_VALUE4	  			0x2B
#define        REG_SYNC_VALUE5	  			0x2C
#define        REG_SYNC_VALUE6	  			0x2D
#define        REG_SYNC_VALUE7	  			0x2E
#define        REG_SYNC_VALUE8	  			0x2F
#define        REG_PACKET_CONFIG1	  		0x30
#define        REG_PACKET_CONFIG2	  		0x31
#define        REG_DETECT_OPTIMIZE			0x31
#define        REG_PAYLOAD_LENGTH_FSK			0x32
#define        REG_NODE_ADRS	  			0x33
#define        REG_BROADCAST_ADRS	 		0x34
#define        REG_FIFO_THRESH	  			0x35
#define        REG_SEQ_CONFIG1	  			0x36
#define        REG_SEQ_CONFIG2	  			0x37
#define        REG_DETECTION_THRESHOLD      		0x37
#define        REG_TIMER_RESOL	  			0x38
#define        REG_TIMER1_COEF	  			0x39
#define        REG_TIMER2_COEF	  			0x3A
#define        REG_IMAGE_CAL	  			0x3B
#define        REG_TEMP		  			0x3C
#define        REG_LOW_BAT	  			0x3D
#define        REG_IRQ_FLAGS1	  			0x3E
#define        REG_IRQ_FLAGS2	  			0x3F
#define        REG_DIO_MAPPING1	  			0x40
#define        REG_DIO_MAPPING2	  			0x41
#define        REG_VERSION	  			0x42
#define        REG_AGC_REF	  			0x43
#define        REG_AGC_THRESH1	  			0x44
#define        REG_AGC_THRESH2	  			0x45
#define        REG_AGC_THRESH3	  			0x46
#define        REG_PLL_HOP	  			0x4B
#define        REG_TCXO		  			0x58
#define        REG_PA_DAC		  		0x5A
#define        REG_PLL		  			0x5C
#define        REG_PLL_LOW_PN	  			0x5E
#define        REG_FORMER_TEMP	  			0x6C
#define        REG_BIT_RATE_FRAC	  		0x70

//LORA MODES:
#define		LORA_SLEEP_MODE				0x80
#define 	LORA_STANDBY_MODE			0x81
#define 	LORA_TX_MODE				0x83
#define 	LORA_RXCONT_MODE			0x85
#define 	LORA_RXSINGLE_MODE			0x86
#define 	LORA_CAD_MODE				0x87
#define 	LORA_MODE_MASK				0x87
#define 	LORA_STANDBY_FSK_REGS_MODE		0xC1

//LORA BANDWIDTH:
#define 	BW_125					0x00
#define 	BW_250					0x40
#define 	BW_500					0x80
#define 	BW_MASK					0xC0

//LORA SPREADING FACTOR:
#define 	SF_6					0x60
#define 	SF_7					0x70
#define 	SF_8					0x80
#define 	SF_9					0x90
#define 	SF_10					0xA0
#define 	SF_11					0xB0
#define 	SF_12					0xC0
#define 	SF_MASK					0xF0

//LORA CODING RATE:
#define 	CR_5					0x08
#define 	CR_6					0x10
#define 	CR_7					0x18
#define 	CR_8					0x20
#define 	CR_MASK					0x38

//CHANNELS
#define		CH_868_200				0xD90CCD		//868.200Mhz
#define		CH_868_225				0xD90E67		//868.225Mhz
#define		CH_868_250				0xD91000		//868.250Mhz
#define		CH_868_275				0xD9119A		//868.275Mhz
#define		CH_868_300				0xD91333		//868.300Mhz
#define		CH_868_325				0xD914CD		//868.325Mhz
#define		CH_868_350				0xD91666		//868.350Mhz
#define		CH_868_375				0xD91800		//868.375Mhz
#define		CH_868_400				0xD9199A		//868.400Mhz
#define		CH_869_525				0xD9619A		//869.525Mhz

//TRANSMITT POWER
#define PA_DAC_HIGH_PWR					0x87
#define PA_DAC_DEFAULT_PWR				0x84

//IRQ FLAGS
#define IRQ_RX_TIMEOUT					0x80
#define IRQ_RX_DONE					0x40
#define IRQ_PAYLOAD_CRC_ERROR				0x20
#define IRQ_VALID_HEADER				0x10
#define IRQ_TX_DONE					0x08
#define IRQ_CAD_DONE					0x04
#define IRQ_FHSS_CHANGE_CHANNEL				0x02
#define IRQ_CAD_DETECTED				0x01

//DIO0 MODES
#define DIO0_RX_DONE					0x00
#define DIO0_TX_DONE					0x40
#define DIO0_CAD_DONE					0x10
#define DIO0_NONE					0xC0

#define LNAGAIN_G1_MAX					0x01
#define LNAGAIN_G2					0x02
#define LNAGAIN_G3					0x03
#define LNAGAIN_G4					0x04
#define LNAGAIN_G5					0x05
#define LNAGAIN_G6_MIN					0x06
#define LNAGAIN_OFFSET					5

// SEND FRAME RETURN VALUES
#define		TX_OK					0
#define		TX_TX_ONGOING				-1
#define		TX_RX_ONGOING				-2

#define		CRC_ON_PAYLOAD				0x40

typedef void (*irq_callback)(int length);

typedef struct
{
	uint32_t channel;
	int dBm;
} sx_region_t;


bool sx1272_init(SPI_HandleTypeDef *spi);

void sx1272_setBandwidth(uint8_t bw);
uint8_t sx1272_getBandwidth(void);

void sx1272_setLowDataRateOptimize(bool ldro);

void sx1272_setPayloadCrc(bool crc);

void sx1272_setSpreadingFactor(uint8_t spr);
uint8_t sx1272_getSpreadingFactor(void);

void sx1272_setCodingRate(uint8_t cr);
uint8_t sx1272_getCodingRate(void);

bool sx1272_setChannel(uint32_t ch);
uint32_t sx1272_getChannel(void);

void sx1272_setExplicitHeader(bool exhdr);
void sx1272_setLnaGain(uint8_t gain, bool lnaboost);

//note: we currently only support PA_BOOST
bool sx1272_setPower(int pwr);

bool sx1272_setArmed(bool rxmode);
bool sx1272_isArmed(void);

void sx1272_irq(void);
void sx1272_setIrqReceiver(irq_callback cb);
void sx1272_clrIrqReceiver(void);

bool sx1272_setRegion(sx_region_t region);

int sx1272_getRssi(void);

int sx1272_sendFrame(uint8_t *data, int length);
int sx1272_receiveFrame(uint8_t *data, int max_length);
bool sx1272_receiveStart(void);
int sx1272_getFrame(uint8_t *data, int max_length);
#endif
