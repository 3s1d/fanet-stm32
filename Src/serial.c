/*
 * serial.c
 *
 *  Created on: May 20, 2017
 *      Author: sid
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "stm32l4xx.h"
#include "stm32l4xx_hal_uart.h"

#include "circular_buffer.h"
#include "serial.h"

/*
 * serial input -> buffer
 */
CIRC_BUF_DEF(serial_uart_rx_buffer, 512);
serial_t serial = {0};

uint8_t serial_uart_inchr = '\0';
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart != serial.uart)
		return;

	circ_buf_push(&serial_uart_rx_buffer, serial_uart_inchr);
	if (serial_uart_inchr == '\n')
	{
		uint32_t prim = __get_PRIMASK();
		__disable_irq();

		serial.num_cmds++;

		if (!prim)
			__enable_irq();
	}
	HAL_UART_Receive_IT(serial.uart, &serial_uart_inchr, 1);
}

serial_t *serial_init(UART_HandleTypeDef *uart)
{
	serial.uart = uart;
	serial.rx_crc_buf = &serial_uart_rx_buffer;

	/* enable uart receiver */
	HAL_UART_Receive_IT(serial.uart, &serial_uart_inchr, 1);

	return &serial;
}

bool serial_poll(serial_t *serial, char *line, int num)
{
//	char buf[64];
//	HAL_UART_Transmit_IT(serial->uart, (uint8_t *)buf,
//			snprintf(buf, sizeof(buf), "h%d t%d\n", serial_uart_rx_buffer.head, serial_uart_rx_buffer.tail), 100);
	if(serial == NULL || serial->num_cmds <= 0)
		return false;

	int pos;
	for(pos=0; circ_buf_pop(serial->rx_crc_buf, (uint8_t *) &line[pos])==0 && pos < num; pos++)
	{
		if(line[pos] == '\n')
			break;
	}

	if(pos > 0 && pos < num)
		line[pos] = '\0';

	/* decrement line count */
	uint32_t prim = __get_PRIMASK();
	__disable_irq();
	serial->num_cmds--;
	if (!prim)
		__enable_irq();

	return true;
}
