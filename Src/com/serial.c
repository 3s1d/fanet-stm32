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
#include "cmd_fanet.h"
#include "serial.h"

UART_HandleTypeDef *serial_uart = NULL;

/*
 * serial input -> buffer
 */
uint8_t serial_uart_inchr = '\0';
CIRC_BUF_DEF(serial_uart_rx_buffer, 512);
int serial_num_cmds = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart != serial_uart)
		return;

	circ_buf_push(&serial_uart_rx_buffer, serial_uart_inchr);
	if (serial_uart_inchr == '\n')
	{
		uint32_t prim = __get_PRIMASK();
		__disable_irq();

		serial_num_cmds++;

		if (!prim)
			__enable_irq();
	}
	HAL_UART_Receive_IT(serial_uart, &serial_uart_inchr, 1);
}

void serial_init(UART_HandleTypeDef *uart)
{
	serial_uart = uart;

	/* enable uart receiver */
	HAL_UART_Receive_IT(serial_uart, &serial_uart_inchr, 1);
}

void serial_reply(char *type, int key, char *msg)
{
	if(serial_uart == NULL || type == NULL)
		return;

	/* general answer */
	if(HAL_UART_Transmit(serial_uart, (uint8_t *)type, strlen(type), 100) != HAL_OK)
		return;

	/* key */
	char buf[64];
	if(key > 0)
	{
		HAL_UART_Transmit(serial_uart, (uint8_t *)buf, snprintf(buf, sizeof(buf), ",%d", key), 100);

		/* human readable message */
		if(msg != NULL)
			HAL_UART_Transmit(serial_uart, (uint8_t *)buf, snprintf(buf, sizeof(buf), ",%s", msg), 100);
	}

	HAL_UART_Transmit(serial_uart, (uint8_t *)buf, snprintf(buf, sizeof(buf), "\n"), 100);
}

bool serial_poll(void)
{
	if(serial_num_cmds <= 0)
		return false;

	char line[256];
	int pos;
	for(pos=0; circ_buf_pop(&serial_uart_rx_buffer, (uint8_t *) &line[pos])==0 && pos < sizeof(line); pos++)
	{
		if(line[pos] == '\n')
			break;
	}

	if(pos > 0 && pos < sizeof(line))
		line[pos] = '\0';

	/* decrement line count */
	uint32_t prim = __get_PRIMASK();
	__disable_irq();
	serial_num_cmds--;
	if (!prim)
		__enable_irq();

	/* eval line */
	if(!strncmp(line, FANET_CMD_START, 3))
		cmd_fanet_mux(line);
	else if(!strncmp(line, DONGLE_CMD_START, 3))
	{
		//dongle_eval(rxStr);
	}
	else
	{
		serial_reply(FN_REPLYE_UNKNOWN_CMD);
	}

	return true;
}
