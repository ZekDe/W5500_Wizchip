/*
 * retarget.c
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */

#include "retarget.h"
#include <stdarg.h>
#include "usbd_cdc_if.h"

void print(const char *format, ...)
{
   va_list va;

   va_start(va, format);

   vsprintf((char*)UserTxBufferFS, format, va);
   uint16_t len = strlen((const char*)UserTxBufferFS);
   CDC_Transmit_FS(UserTxBufferFS, len);
   while (!usb_tx_len);
   usb_tx_len = 0;

   va_end(va);
}

int scan(const char *format, ... )
{
   va_list va;

   if (!usb_rx_len)
      return 0;

   va_start(va, format);
   int len = vsscanf((const char*)UserRxBufferFS, format, va);

   va_end(va);
   usb_rx_len = 0;

   return len;
}

uint16_t getline(char *buf)
{
   uint16_t i;

   if (!usb_rx_len)
         return 0;

   for (i = 0; i < usb_rx_len; i++)
   {
       if (UserRxBufferFS[i] == '\n' || UserRxBufferFS[i] == '\0')
           break;

       buf[i] = UserRxBufferFS[i];
   }
   buf[i] = '\0';

   usb_rx_len = 0;
   return i-1;
}

void flush(void)
{
   for(uint16_t i = 0; i < usb_rx_len; i++)
      UserRxBufferFS[i] = 0;
}
