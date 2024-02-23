/*
 * socket_utils.h
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */

#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include "stdint.h"

void setSockIntCallbacks(uint8_t id,
      void (*send_ok)(uint8_t id, uint16_t len),
      void (*timeout)(uint8_t id, uint8_t discon),
      void (*recv)(uint8_t id),
      void (*discon)(uint8_t id),
      void (*con)(uint8_t id));


void setEthIntCallbacks(
      void (*conflict)(uint8_t id),
      void (*unreach)(uint8_t id),
      void (*PPPoe)(uint8_t id),
      void (*mp)(uint8_t id));

void setSockIntErrCallbacks(uint8_t id, void (*hardfault)(uint8_t id));
void ethIntErrCallbacks(void (*linkoff)(void));


// call periodically medium or high frequency
void sockDataHandler(uint8_t id);
// put in ISR
void ethIntAsserted(void);
//call periodically low frequency
void checkEthHealth(void);

#endif /* SOCKET_UTILS_H_ */
