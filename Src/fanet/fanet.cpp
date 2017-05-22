/*
 * fanet.cpp
 *
 *  Created on: May 22, 2017
 *      Author: sid
 */


#include "serial.h"
#include "radio/fmac.h"
#include "wire/serial_interface.h"
#include "app.h"

void fanet_init(serial_t *serial)
{
	serial_int.begin(serial);
	app.begin(serial_int);
	while(fmac.begin(app) == false)
	{
		serial_int.print_line(FN_REPLYE_RADIO_FAILED);
		HAL_Delay(1000);
	}
	serial_int.print_line(FN_REPLYM_INITIALIZED);

}

void fanet_loop(void)
{
	/* get commands from serial */
	serial_int.handle_rx();

	/* update MAC state machine */
	fmac.handle();
}
