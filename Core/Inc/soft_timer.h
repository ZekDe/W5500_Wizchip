#ifndef SOFT_TIMER_H
#define SOFT_TIMER_H

#include "stdint.h"

typedef struct
{
  uint8_t in;
  uint8_t aux;
  void(*fp)(void);
  uint32_t interval;
  uint32_t since;
} soft_timer_t;

void timerCheck(soft_timer_t *obj, uint32_t now);
void timerStart(soft_timer_t *obj);
void timerStop(soft_timer_t *obj);
void timerSet(soft_timer_t *obj, uint32_t interval, void(*cb)(void));

#endif /* SOFT_TIMER_H */
