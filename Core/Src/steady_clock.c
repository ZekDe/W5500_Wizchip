/*
 * steady_clock.c
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */

#include "steady_clock.h"
#include "main.h"

void dwt_enable(void)
{
//24.Bit TRCENA in Debug Exception and Monitor Control Register must be set before enable DWT
   CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
// CYCCNT is a free running counter, counting upwards.32 bits. 2^32 / cpuFreq = max time
   DWT->CYCCNT = 0u;// initialize the counter to 0
   DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;// Enable the CYCCNT counter.
}

float getusec(void)
{
   return DWT->CYCCNT / (float)(SystemCoreClock / 1000000);
}
