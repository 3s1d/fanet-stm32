/*
 * fanet.cpp
 *
 *  Created on: May 22, 2017
 *      Author: sid
 */


#include "radio/fmac.h"
#include "radio/sx1272.h"
#include "wire/serial.h"
#include "wire/serial_interface.h"
#include "app.h"
#include "fanet.h"

#ifdef FLARM
#include "../flarm/caswrapper.h"
#endif

static uint8_t fanet_sxirq = 0;
static uint8_t fanet_sxexec = 0;

#ifdef FLARM
	static uint16_t flarm_ppsirq = 0;
	static uint16_t flarm_ppsexec = 0;
#endif

void fanet_init(serial_t *serial)
{
	serial_int.begin(serial);

	/* FANET */
	app.begin(serial_int);
	while(fmac.begin(app) == false)
	{
		serial_int.print_line(FN_REPLYE_RADIO_FAILED);
		HAL_Delay(500);
		NVIC_SystemReset();
	}

	/* FLARM */
#ifdef FLARM
	casw.begin(fmac.myAddr);
#endif

	serial_int.print_line(FN_REPLYM_INITIALIZED);
}

void fanet_sx_int(void)
{
	fanet_sxirq++;
}

void fanet_pps_int(void)
{
#ifdef FLARM
	flarm_ppsirq++;
#endif
}


void fanet_loop(void)
{
	/* get commands from serial */
	serial_int.handle_rx();

	/* update MAC state machine */
	if(fanet_sxirq != fanet_sxexec)
	{
		fanet_sxexec++;
		sx1272_irq();
	}
	fmac.handle();

	/* FLARM */
#ifdef FLARM
	if(flarm_ppsirq != flarm_ppsexec)
	{
		flarm_ppsexec++;
		casw.pps();
	}

	casw.handle();
#endif
}
