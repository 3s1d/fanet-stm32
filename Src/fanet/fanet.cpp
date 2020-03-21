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
	static uint16_t casw_ppsIrq = 0;
	static uint16_t casw_ppsExec = 0;
	static uint32_t casw_ppsTstamp = 0;
	static uint32_t casw_nextPps = UINT32_MAX;
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
	casw_ppsIrq++;
	casw_ppsTstamp = HAL_GetTick();
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

	/* execute scheduled pps */
	uint32_t t = HAL_GetTick();
	if(t >= casw_nextPps)
	{
		casw.pps();
		casw.handle(t);
		casw_nextPps = UINT32_MAX;
	}

	/* pps signal */
	if(casw_ppsIrq != casw_ppsExec)
	{
		casw_ppsExec++;

		/* get pps timestamp */
		uint32_t prim = __get_PRIMASK();
		__disable_irq();
		volatile uint32_t local_tstamp = casw_ppsTstamp;
		if (!prim)
			__enable_irq();

		/* pps not yet happend, trigger now */
		if(casw_nextPps != UINT32_MAX)
		{
			casw.pps();
			casw.handle(t);
		}

		/* bring forward pps by 15ms */
		casw_nextPps = local_tstamp + 985;
	}

	/* regular execution */
	casw.handle(HAL_GetTick());
#endif
}
