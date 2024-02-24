/*
 * socket_utils.h
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */

#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

/*
 * IMR(IP Conflict ve Destination unreachable) - IR(yandakilerden hangisi)
 * SIMR(S0_IMR enable) - SIR(S0_INT bak) - Sn_IR(SEND_OK, TIMEOUT, RECV, DISCON, CON)
 * Sn_IMR(SEND_OK, TIMEOUT, RECV, DISCON, CON) - Sn_IR(yandakilerden hangisi)
 */

#include "stdint.h"

typedef struct wiz_NetInfo_t wiz_NetInfo;

void ethInit(const wiz_NetInfo *info, const uint8_t *txsize, const uint8_t *rxsize);
void ethInitDefault(const wiz_NetInfo *info);
void setSockIntCallbacks(uint8_t sn,
      void (*send_ok)(uint8_t sn, uint16_t len),
      void (*timeout)(uint8_t sn, uint8_t discon),
      void (*recv)(uint8_t sn),
      void (*discon)(uint8_t sn),
      void (*con)(uint8_t sn));
void setEthIntCallbacks(
      void (*conflict)(void),
      void (*unreach)(void),
      void (*PPPoe)(void),
      void (*mp)(void));
void setSockIntErrCallbacks(uint8_t sn, void (*hardfault)(uint8_t sn));
void setEthIntErrCallbacks(void (*linkoff)(void));
void setTCPto(uint16_t rtr, uint8_t rcr);
void enableKeepAliveAuto(uint8_t sn, uint8_t val);
void sockDataHandler(uint8_t sn);
void ethIntAsserted(void);
void ethObserver(void);

#endif /* SOCKET_UTILS_H_ */
