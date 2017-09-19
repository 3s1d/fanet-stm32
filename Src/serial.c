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
		serial.pushed_cmds++;

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
	if(serial == NULL)
		return false;

	/* ensure receiver is on */
	HAL_UART_Receive_IT(serial->uart, &serial_uart_inchr, 1);

	if(serial->pushed_cmds == serial->pulled_cmds)
		return false;

	int pos;
	for(pos=0; circ_buf_pop(serial->rx_crc_buf, (uint8_t *) &line[pos])==0 && pos < num; pos++)
	{
		if(line[pos] == '\n')
		{
			serial->pulled_cmds++;
			break;
		}
	}

	if(pos >= 0 && pos < num)
		line[pos] = '\0';

	return true;
}

void serial_print(serial_t *ser, char *str)
{
	/* use default */
	if(ser == NULL)
		ser = &serial;

	if(ser == NULL || str == NULL)
		return;

	while(HAL_UART_Transmit_IT(ser->uart, (uint8_t *)str, strlen(str)) == HAL_BUSY);
	while(ser->uart->gState != (__IO HAL_UART_StateTypeDef) HAL_UART_STATE_READY);
}
