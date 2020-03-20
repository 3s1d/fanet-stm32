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
CIRC_BUF_DEF(serial_uart_rx_buffer, 768);
serial_t serial = {0};

uint8_t serial_uart_fifo[256];
uint16_t serial_uart_idx = 0;

void serialRxCallback(UART_HandleTypeDef *huart)
{
	if(huart != serial.uart)
		return;

	/* initial state ignore */
	if(__HAL_DMA_GET_COUNTER(huart->hdmarx) == 0)
		return;

	while(serial_uart_idx != (sizeof(serial_uart_fifo) - __HAL_DMA_GET_COUNTER(huart->hdmarx)))
	{
		/* add to ring buffer and check for line end */
		if (circ_buf_push(&serial_uart_rx_buffer, serial_uart_fifo[serial_uart_idx]) == 0 && serial_uart_fifo[serial_uart_idx] == '\n')
			serial.pushed_cmds++;

		/* advance ring index */
		if(++serial_uart_idx >= sizeof(serial_uart_fifo))
			serial_uart_idx -= sizeof(serial_uart_fifo);
	}
}

serial_t *serial_init(UART_HandleTypeDef *uart)
{
	serial.uart = uart;
	serial.rx_crc_buf = &serial_uart_rx_buffer;

	serial.uart = uart;
	serial.uart->RxCpltCallback = serialRxCallback;
	serial.uart->RxHalfCpltCallback = serialRxCallback;
	serial.rx_crc_buf = &serial_uart_rx_buffer;

	/* enable uart receiver */
	__HAL_UART_ENABLE_IT(serial.uart, UART_IT_IDLE);
	HAL_UART_Receive_DMA(serial.uart, serial_uart_fifo, sizeof(serial_uart_fifo));

	return &serial;
}

bool serial_poll(serial_t *serial, char *line, int num)
{
	if(serial == NULL || line == NULL || num == 0)
		return false;

	if(serial->pushed_cmds == serial->pulled_cmds)
		return false;

	int pos;
	for(pos=0; pos < num && circ_buf_pop(serial->rx_crc_buf, (uint8_t *) &line[pos])==0; pos++)
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
