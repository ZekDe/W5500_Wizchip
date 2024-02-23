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

void timer_check(soft_timer_t *obj, uint32_t now);
void timer_start(soft_timer_t *obj);
void timer_stop(soft_timer_t *obj);
void timer_set(soft_timer_t *obj, uint32_t interval, void(*cb)(void));

#endif /* SOFT_TIMER_H */
