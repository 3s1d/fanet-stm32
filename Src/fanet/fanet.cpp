/*
 * fanet.cpp
 *
 *  Created on: May 22, 2017
 *      Author: sid
 */


#include "serial.h"
#include "radio/fmac.h"
#include "radio/sx1272.h"
#include "wire/serial_interface.h"
#include "app.h"
#include "fanet.h"

#ifdef FLARM
#include "../flarm/caswrapper.h"
#endif

static uint8_t fanet_sxirq = 0;
static uint8_t fanet_sxexec = 0;

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
	casw.pps();
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
	casw.handle();
#endif
}
