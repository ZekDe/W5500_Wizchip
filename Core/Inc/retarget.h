/*
 * retarget.h
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */

#ifndef RETARGET_H_
#define RETARGET_H_

#include "stdint.h"

void print(const char *format, ...);
int scan(const char *format, ... );
uint16_t getline(char *buf);

#endif /* RETARGET_H_ */
