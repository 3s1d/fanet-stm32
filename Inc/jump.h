/*
 * jump.h
 *
 *  Created on: May 13, 2017
 *      Author: sid
 */

#ifndef BOOTLOADER_JUMP_H_
#define BOOTLOADER_JUMP_H_

#ifdef __cplusplus
extern "C" {
#endif


void jump_app(void* startAddr);
void jump_mcu_bootloader(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_JUMP_H_ */
