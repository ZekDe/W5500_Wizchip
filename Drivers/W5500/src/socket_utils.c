/*
 * socket_utils.c
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */
#include "socket_utils.h"
#include "socket.h"

typedef struct
{
   uint32_t send_ok :1;
   uint32_t timeout :1;
   uint32_t recv :1;
   uint32_t discon :1;
   uint32_t con :1;
} sock_int_status_t;

typedef struct
{
   void (*send_ok)(uint8_t id, uint16_t len);
   void (*timeout)(uint8_t id, uint8_t discon);
   void (*recv)(uint8_t id);
   void (*discon)(uint8_t id);
   void (*con)(uint8_t id);
}sock_int_status_cb_t;

typedef struct
{
   uint32_t conflict :1;
   uint32_t unreach :1;
   uint32_t PPPoe :1;
   uint32_t mp :1;
}eth_int_status_t;

typedef struct
{
   void (*conflict)(uint8_t id);
   void (*unreach)(uint8_t id);
   void (*PPPoe)(uint8_t id);
   void (*mp)(uint8_t id);
}eth_int_status_cb_t;


typedef struct
{
   uint32_t discon :1;
   uint32_t hardfault :1;
}sock_int_err_t;

typedef struct
{
   void (*hardfault)(uint8_t id);
}sock_int_err_cb_t;

typedef struct
{
   void (*linkoff)(void);
}eth_int_err_cb_t;

enum id{
   SOCK_0,
   SOCK_END
};

sock_int_status_t sock_int_status[SOCK_END];
sock_int_status_cb_t sock_int_status_cb[SOCK_END];

eth_int_status_t  eth_int_status;
eth_int_status_cb_t eth_int_status_cb;

sock_int_err_t sock_int_err[SOCK_END];
sock_int_err_cb_t sock_int_err_cb[SOCK_END];

eth_int_err_cb_t eth_int_err_cb;

volatile uint8_t eth_flag;


void setSockIntCallbacks(uint8_t id,
      void (*send_ok)(uint8_t id, uint16_t len),
      void (*timeout)(uint8_t id, uint8_t discon),
      void (*recv)(uint8_t id),
      void (*discon)(uint8_t id),
      void (*con)(uint8_t id))
{
   sock_int_status_cb[id].send_ok = send_ok;
   sock_int_status_cb[id].timeout = timeout;
   sock_int_status_cb[id].recv = recv;
   sock_int_status_cb[id].discon = discon;
   sock_int_status_cb[id].con = con;
}

void setEthIntCallbacks(
      void (*conflict)(uint8_t id),
      void (*unreach)(uint8_t id),
      void (*PPPoe)(uint8_t id),
      void (*mp)(uint8_t id))
{
   eth_int_status_cb.conflict = conflict;
   eth_int_status_cb.unreach = unreach;
   eth_int_status_cb.PPPoe = PPPoe;
   eth_int_status_cb.mp = mp;
}

void setSockIntErrCallbacks(uint8_t id,
      void (*hardfault)(uint8_t id))
{
   sock_int_err_cb[id].hardfault = hardfault;
}

void ethIntErrCallbacks(void (*linkoff)(void))
{
   eth_int_err_cb.linkoff = linkoff;
}



void sockDataHandler(uint8_t id)
{
   if(!eth_flag)
      return;

   eth_flag = 0;

   uint8_t snir = getSn_IR(id);
   uint8_t ir = getIR();
   sock_int_status[id].send_ok = !!(snir & Sn_IR_SENDOK);
   sock_int_status[id].timeout = !!(snir & Sn_IR_TIMEOUT);
   sock_int_status[id].recv = !!(snir & Sn_IR_RECV);
   sock_int_status[id].discon = !!(snir & Sn_IR_DISCON);
   sock_int_status[id].con = !!(snir & Sn_IR_CON);
   eth_int_status.conflict = !!(ir & IR_CONFLICT);
   eth_int_status.unreach = !!(ir & IR_UNREACH);

   if(sock_int_status[id].con)
   {
      setSn_IR(id, Sn_IR_CON);
      sock_int_status_cb[id].con(id);
   }
   else if(sock_int_status[id].discon)
   {
      setSn_IR(id, Sn_IR_DISCON);

      if(disconnect(id) != SOCK_OK)
      {
         return;
      }

      sock_int_status_cb[id].discon(id);
   }
   else if(sock_int_status[id].timeout)
   {
      setSn_IR(id, Sn_IR_TIMEOUT);

      if(getSn_CR(id) & Sn_CR_DISCON)
      {
         if(close(id) == SOCK_OK)
            sock_int_status_cb[id].timeout(id, 1);
         else
            sock_int_err_cb[id].hardfault(id);

      }
      sock_int_status_cb[id].timeout(id, 0);
   }
   else if(sock_int_status[id].recv)
   {
      setSn_IR(id, Sn_IR_RECV);
      sock_int_status_cb[id].recv(id);
   }
   else if(sock_int_status[id].send_ok)
   {
      setSn_IR(id, Sn_IR_SENDOK);
      uint16_t len = getSn_TxMAX(id) - getSn_TX_FSR(id);
      sock_int_status_cb[id].send_ok(id, len);
   }

}

void ethIntAsserted(void)
{
   eth_flag = 1;
}

void checkEthHealth(void)
{
// disconnected durumunda da kablo kontrolünü yapabiliyor
   !(getPHYCFGR() & PHYCFGR_LNK_ON) && (eth_int_err_cb.linkoff(), 0);

   /* disconnect komutu gönderildi, başarısız çıkış oldu, sebebi timeout mu?
    * disconnect geldiğinde, timeout oluştuğunda halen interrupta giriyorsa
    * aşağıdaki bu durum "ethIntDataHandler" fonksiyonu altında
    *  timeout interrupt'ı içerisinde ele alınacak. Timeout durumunu simüle edemedim
    *  büyük ihtimal halen timeout int çalışıyor olacak çünkü henüz SOCK_CLOSE oluşmadı
    */
//   if((getSn_CR(sn) & Sn_CR_DISCON) && (getSn_IR(sn) & Sn_IR_TIMEOUT))
//   {
//      eth_int_err[id].discon = 0;
//      if(close(0) == SOCK_OK)
//         sock_cb[id].timeout(id);
//      else
//         eth_int_err[id].hardfault = 1;
//   }

}



