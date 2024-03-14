/*
 * socket_utils.c
 *
 *  Created on: Feb 23, 2024
 *      Author: Duatepe
 */
#include "socket_utils.h"
#include "socket.h"
#include "macro.h"
#include "edge_detection.h"
#include "main.h" // for tick


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
   void (*send_ok)(uint8_t sn, uint16_t len);
   void (*timeout)(uint8_t sn, uint8_t discon);
   void (*recv)(uint8_t sn, uint16_t len);
   void (*discon)(uint8_t sn);
   void (*con)(uint8_t sn);
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
   void (*conflict)(void);
   void (*unreach)(void);
   void (*PPPoe)(void);
   void (*mp)(void);
}eth_int_status_cb_t;


typedef struct
{
   void (*ethfault)(uint8_t sn);
}sock_int_err_cb_t;

typedef struct
{
   void (*linkoff)(void);
}eth_int_err_cb_t;

enum{
   SOCK_0,
   SOCK_END,
};

enum
{
   ED_LINKOFF,
   ED_END,
};


static sock_int_status_cb_t sock_int_status_cb[SOCK_END];

static eth_int_status_t  eth_int_status;
static eth_int_status_cb_t eth_int_status_cb;

static sock_int_err_cb_t sock_int_err_cb[SOCK_END];

static eth_int_err_cb_t eth_int_err_cb;

static volatile uint8_t eth_flag;

static edge_detection_t ed_obj[ED_END];

/**
 * \fn void ethInit(wiz_NetInfo*, uint8_t*, uint8_t*)
 * \brief must be called first
 * \param info
 * \param txsize
 * \param rxsize
 */
void ethInit(wiz_NetInfo* info, uint8_t* txsize, uint8_t* rxsize)
{
   wizchip_init(txsize, rxsize);
   wizchip_setnetinfo(info);
}

/**
 * \fn void ethInitDefault(wiz_NetInfo*)
 * \brief must be called first
 * \param info
 */
void ethInitDefault(wiz_NetInfo* info)
{
   uint8_t sock_buffer[] = {16, 0, 0, 0, 0, 0, 0, 0};
   ethInit(info, sock_buffer, sock_buffer);
   setTCPto(2000, 5);
}


/**
 * \fn void setSockIntCallbacks(uint8_t, void(*)(uint8_t, uint16_t), void(*)(uint8_t, uint8_t), void(*)(uint8_t), void(*)(uint8_t), void(*)(uint8_t))
 * \brief setter. If callbacks functions are not NULL then related interrupt is enabled,
 * and global interrupt is enabled for sn socket.
 * \param sn: socket number
 * \param send_ok: called in case of data send process done
 * \param timeout: this function also takes "discon" parameter.
 * discon: 0: normal timeout like cable unplugged.
 *         1: disconnect command executed, socket couldn't be closed yet.
 *         2: cannot connect to host.
 * \param recv: called in case of data received
 * \param discon: called in case of disconnection
 * \param con: called in case of connection
 */
void setSockIntCallbacks(uint8_t sn,
      void (*send_ok)(uint8_t sn, uint16_t len),
      void (*timeout)(uint8_t sn, uint8_t discon),
      void (*recv)(uint8_t sn, uint16_t len),
      void (*discon)(uint8_t sn),
      void (*con)(uint8_t sn))
{

   uint8_t reg = 0;

   if(send_ok)
      SET_BIT_POS(reg, 4);
   if(timeout)
      SET_BIT_POS(reg, 3);
   if(recv)
      SET_BIT_POS(reg, 2);
   if(discon)
      SET_BIT_POS(reg, 1);
   if(con)
      SET_BIT_POS(reg, 0);

   setSn_IMR(sn, reg);
   reg = 0;


   reg = getSIMR();
   SET_BIT_POS(reg, sn);
   setSIMR(reg);

   sock_int_status_cb[sn].send_ok = send_ok;
   sock_int_status_cb[sn].timeout = timeout;
   sock_int_status_cb[sn].recv = recv;
   sock_int_status_cb[sn].discon = discon;
   sock_int_status_cb[sn].con = con;
}

/**
 * \fn void setEthIntCallbacks(void(*)(void), void(*)(void), void(*)(void), void(*)(void))
 * \brief If callbacks functions are not NULL then related interrupt enable.
 * \param conflict:
 * \param unreach
 * \param PPPoe
 * \param mp
 */
void setEthIntCallbacks(
      void (*conflict)(void),
      void (*unreach)(void),
      void (*PPPoe)(void),
      void (*mp)(void))
{
   uint8_t reg = 0;

   if(conflict)
      reg |= IM_IR7;
   if(unreach)
      reg |= IM_IR6;
   if(PPPoe)
      reg |= IM_IR5;
   if(mp)
      reg |= IM_IR4;

   setIMR(reg);

   eth_int_status_cb.conflict = conflict;
   eth_int_status_cb.unreach = unreach;
   eth_int_status_cb.PPPoe = PPPoe;
   eth_int_status_cb.mp = mp;
}

/**
 * \fn void setSockIntErrCallbacks(uint8_t, void(*)(uint8_t))
 * \brief setter for ethfault function
 * \param sn: socket number
 * \param ethfault: it is called in case of if device registers cannot be read.
 * actually in this file, this function is called if disconnect cannot be executed correctly
 * which depends device register couldn't be read in background.
 */
void setSockIntErrCallbacks(uint8_t sn,
      void (*ethfault)(uint8_t sn))
{
   sock_int_err_cb[sn].ethfault = ethfault;
}

/**
 * \fn void setEthIntErrCallbacks(void(*)(void))
 * \brief setter for link-off function
 * \param linkoff: function is called in case of link-off
 */
void setEthIntErrCallbacks(void (*linkoff)(void))
{
   eth_int_err_cb.linkoff = linkoff;

}

/**
 * \fn void setTCPto(uint16_t, uint8_t)
 * \brief datasheet page 39 for calculation.
 * Configures the total retransmission timeout
 * \param rtr: retry-time value
 * \param rcr: retry-count value
 */
void setTCPto(uint16_t rtr, uint8_t rcr)
{
   setRTR(rtr);
   setRCR(rcr);
}


/**
 * \fn void enableKeepAliveAuto(uint8_t, uint8_t)
 * \brief this function should be called after the socket has been created
 * \param sn: socket no
 * \param val: 1 unit is 5 second
 */
void enableKeepAliveAuto(uint8_t sn, uint8_t val)
{
   //setSn_CR(sn, Sn_CR_SEND_KEEP);
   setSn_KPALVTR(sn, val);
}

int8_t isConnected(uint8_t sn)
{
   return getSn_SR(sn) == SOCK_ESTABLISHED;
}

int8_t isClosed(uint8_t sn)
{
   return getSn_SR(sn) == SOCK_CLOSED;
}

int8_t isOpened(uint8_t sn)
{
   return getSn_SR(sn) == SOCK_INIT;
}
/**
 * \fn void sockDataHandler(uint8_t)
 * \brief call it periodically
 * mostly good for medium or high frequency
 * \param sn: socket no
 */
void sockDataHandler(uint8_t sn)
{
   if(!eth_flag)
      return;

   eth_flag = 0;

   // interrrupt ayrıştırılması BEGIN
   uint8_t snir = getSn_IR(sn);
   uint8_t ir = getIR();
   sock_int_status_t sock_int_status;

   sock_int_status.send_ok = !!(snir & Sn_IR_SENDOK);
   sock_int_status.timeout = !!(snir & Sn_IR_TIMEOUT);
   sock_int_status.recv = !!(snir & Sn_IR_RECV);
   sock_int_status.discon = !!(snir & Sn_IR_DISCON);
   sock_int_status.con = !!(snir & Sn_IR_CON);
   eth_int_status.conflict = !!(ir & IR_CONFLICT);
   eth_int_status.unreach = !!(ir & IR_UNREACH);
   eth_int_status.PPPoe = !!(ir & IR_PPPoE);
   eth_int_status.mp = !!(ir & IR_MP);

   // interrrupt ayrıştırılması END

   // ayrıştırılma sonucunun değerlendirilmesi BEGIN
   if(sock_int_status.con)
   {
      setSn_IR(sn, Sn_IR_CON);
      sock_cmd[sn].connect = 0;
      sock_int_status_cb[sn].con(sn);
   }
   else if(sock_int_status.discon)
   {
      setSn_IR(sn, Sn_IR_DISCON);
      sock_cmd[sn].discon = 0;
      sock_int_status_cb[sn].discon(sn);
   }
   else if(sock_int_status.timeout)
   {
      setSn_IR(sn, Sn_IR_TIMEOUT);

      if(sock_cmd[sn].discon)
      {
         sock_cmd[sn].discon = 0;
         if(close(sn) == SOCK_OK)
            sock_int_status_cb[sn].timeout(sn, 1);
         else
            sock_int_err_cb[sn].ethfault(sn);
      }
      else if(sock_cmd[sn].connect)
      {
         sock_cmd[sn].connect = 0;
         sock_int_status_cb[sn].timeout(sn, 2);
      }
      else
         sock_int_status_cb[sn].timeout(sn, 0);

   }
   else if(sock_int_status.recv)
   {
      setSn_IR(sn, Sn_IR_RECV);
      sock_int_status_cb[sn].recv(sn, getSn_RX_RSR(sn));
   }
   else if(sock_int_status.send_ok)
   {
      setSn_IR(sn, Sn_IR_SENDOK);
      sock_int_status_cb[sn].send_ok(sn, getSn_TxMAX(sn) - getSn_TX_FSR(sn));
   }
   else if(eth_int_status.conflict)
   {
      setIR(IR_CONFLICT);
      eth_int_status_cb.conflict();
   }
   else if(eth_int_status.unreach)
   {
      setIR(IR_UNREACH);
      eth_int_status_cb.unreach();
   }
   else if(eth_int_status.PPPoe)
   {
      setIR(IR_PPPoE);
      eth_int_status_cb.PPPoe();
   }
   else if(eth_int_status.mp)
   {
      setIR(IR_MP);
      eth_int_status_cb.mp();
   }
   // ayrıştırılma sonucunun değerlendirilmesi END

}

/**
 * \fn void ethIntAsserted(void)
 * \brief put in ISR
 */
void ethIntAsserted(void)
{
   eth_flag = 1;
}


/**
 * \fn void ethObserver(uint8_t sn)
 * \brief call periodically in medium frequency like 50 ms
 * you can make your own methods to observe something about connection
 */
void ethObserver(uint8_t sn)
{
   edgeDetection(&ed_obj[ED_LINKOFF], !(getPHYCFGR() & PHYCFGR_LNK_ON)) && (eth_int_err_cb.linkoff(), 0);


   if(sock_cmd[sn].connect)
   {
      if(getSn_SR(sn) == SOCK_CLOSED)
      {
         sock_cmd[sn].connect = 0;
      }
   }
}

