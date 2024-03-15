/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include "socket.h"
#include "w5500.h"
#include "wizchip_conf.h"
#include "loopback.h"
#include "ton.h"
#include "soft_timer.h"
#include "socket_utils.h"
#include "string.h"
#include "retarget.h"
#include "steady_clock.h"
#include "macro.h"

#include "../../Drivers/Internet/inc/dhcp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

enum
{
   TON_0_DURATION = 5000,
};

enum
{
   TON_0 = 0,
   TON_END,
};

enum
{
   TIMER_0_DURATION = 50,
};

enum
{
   TIMER_0 = 0,
   TIMER_END,
};

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

/* USER CODE BEGIN PV */

typedef struct
{
   wiz_NetInfo net_info;
   uint16_t port;
   uint16_t destport;
   uint8_t sn;
   uint8_t destip[4];
   uint8_t dhcp_buf[1024];
} app_data_t;

app_data_t app_net_data =
{
  .net_info =
  {
      .mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},// Mac address
      .ip = {192, 168, 1, 99},// IP address
      .sn = {255, 255, 255, 0},// Subnet mask
      .gw = {192, 168, 1, 1},// Gateway address
   },
   .port = 8080,
   .sn = 0,
   .destport = 50000,
   .destip = {192, 168, 1, 34}, };

//app_data_t app_net_data =
//{
//  .net_info =
//  {
//      .mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},// Mac address
//      .ip = {10, 60, 3, 99},// IP address
//      .sn = {255, 0, 0, 0},// Subnet mask
//      .gw = {10, 60, 3, 1}// Gateway address
//   },
//   .port = 8080,
//   .sn = 0,
//   .destport = 50000,
//   .destip = {10, 60, 3, 20},
//};

static soft_timer_t timer_obj[TIMER_END];
static ton_t ton_obj[TON_END];

volatile static uint8_t spi_rx_done;
volatile static uint8_t spi_tx_done;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */
static void printInfo(void);

static void doSendOk(uint8_t sn, uint16_t len);
static void doTimeout(uint8_t sn, uint8_t discon);
static void doRecv(uint8_t sn, uint16_t len);
static void doDiscon(uint8_t sn);
static void doCon(uint8_t sn);
static void doEthfault(uint8_t sn);
static void doLinkOff(void);
static void doConflict(void);
static void doUnreach(void);

static void DHCP_Perform(void);

static void periodicTimer(void);

static void cs_sel(void);
static void cs_desel(void);
static void spi_rb_burst(uint8_t *rbuf, uint16_t len);
static void spi_wb_burst(uint8_t *tbuf, uint16_t len);
static void spi_rb_burst_dma(uint8_t *rbuf, uint16_t len);
static void spi_wb_burst_dma(uint8_t *tbuf, uint16_t len);

uint8_t buf[16384] =
{0};
uint8_t cmd[15] ={0};
uint32_t first,second,result;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
   /* USER CODE BEGIN 1 */

   /* USER CODE END 1 */

   /* MCU Configuration--------------------------------------------------------*/

   /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
   HAL_Init();

   /* USER CODE BEGIN Init */

   /* USER CODE END Init */

   /* Configure the system clock */
   SystemClock_Config();

   /* USER CODE BEGIN SysInit */

   /* USER CODE END SysInit */

   /* Initialize all configured peripherals */
   MX_GPIO_Init();
   MX_DMA_Init();
   MX_SPI2_Init();
   MX_USB_DEVICE_Init();
   /* USER CODE BEGIN 2 */

// RESET HIGH(active low 500 us)
   HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_RESET);
   HAL_Delay(10);
   HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_SET);
   HAL_Delay(10);

   reg_wizchip_cs_cbfunc(cs_sel, cs_desel);
   reg_wizchip_spiburst_cbfunc(spi_rb_burst_dma, spi_wb_burst_dma);

// ethInitDefault ilk çağrılmalı
   ethInitDefault(&app_net_data.net_info);
   setEthIntCallbacks(doConflict, doUnreach, NULL, NULL);
   setSockIntCallbacks(app_net_data.sn, doSendOk, doTimeout, doRecv, doDiscon, doCon);
   setSockIntErrCallbacks(app_net_data.sn, doEthfault);
   setEthIntErrCallbacks(doLinkOff);

   timerSet(&timer_obj[TIMER_0], TIMER_0_DURATION, periodicTimer);

   while (!scan("%s", cmd));

   print("%s\n", cmd);
   CLEAR_STRUCT(cmd);

   printInfo();
   print("CHIP-VERSION: %d\n"
         "INTLEVEL: %d\n"
         "IMR: %X\n"
         "SIMR: %X\n"
         "RTR: %d\n"
         "RCR: %d\n"
         "Sn_IMR: %X\n"
         "TX Buffer: %d kb\n"
         "RX Buffer: %d kb\n"
         "Link Status: %s\n", getVERSIONR(), getINTLEVEL(), getIMR(), getSIMR(), getRTR(), getRCR(), getSn_IMR(0), getSn_TXBUF_SIZE(0),
         getSn_RXBUF_SIZE(0),
         getPHYCFGR() & PHYCFGR_LNK_ON ? "ON" : "OFF");

   timerStart(&timer_obj[TIMER_0]);

   steadyClockEnable();

   DHCP_Perform();


   /* USER CODE END 2 */

   /* Infinite loop */
   /* USER CODE BEGIN WHILE */

   while (1)
   {
      sockDataHandler(app_net_data.sn);


      if(*cmd)
         CLEAR_STRUCT(cmd);

      getline((char*)cmd);

      if(!strcmp("socket", (char*)cmd))
      {
         first = getusec();
         int8_t sockret;
         if((sockret = socket(app_net_data.sn, Sn_MR_TCP, app_net_data.port, 0x00)) != app_net_data.sn)
         {
            print("socket %d cannot open\r\n"
                  "sockret: %d\n", app_net_data.sn, sockret);
         }
         second = getusec();
         print("socket duration: %d\n",second - first);
         enableKeepAliveAuto(app_net_data.sn, 2);
         print("Socket %d opened\r\n", app_net_data.sn);

      }
      else if(!strcmp("listen", (char*)cmd))
      {
         first = getusec();
         if(listen(app_net_data.sn) != SOCK_OK)
         {
            print("socket %d cannot listen\n", app_net_data.sn);
         }
         second = getusec();
         print("listen duration: %d\n",second - first);
         print("Listen: Socket [%d], Port [%d]\r\n", app_net_data.sn, app_net_data.port);
      }
      else if(!strcmp("connect", (char*)cmd))
      {
         print("Socket %d try to connect to the %d.%d.%d.%d : %d\n", app_net_data.sn, app_net_data.destip[0], app_net_data.destip[1],
               app_net_data.destip[2], app_net_data.destip[3], app_net_data.destport);

         int8_t ret = 0;
         first = getusec();
         if((ret = connect(app_net_data.sn, app_net_data.destip, app_net_data.destport)) != SOCK_OK)
            print("connection failed: %d\n", ret);
         second = getusec();
         print("connect duration: %d\n", second - first);
      }
      else if(!strcmp("disconnect", (char*)cmd))
      {
         first = getusec();
         if(disconnect(app_net_data.sn) != SOCK_OK)
         {
            print("cannot disconnect properly!\n");
         }
         second = getusec();
         print("disconnect duration: %d\n", second - first);
      }
      else if(!strcmp("send", (char*)cmd))
      {
         first = getusec();
         if(send(0, (uint8_t*)"bir yazi\n", 9) < 0)
         {
            print("cannot send!\n");
         }
         second = getusec();
         print("send duration: %d\n", second - first);
      }
      else if(!strcmp("16k", (char*)cmd))
      {
         send(0, (uint8_t*)"16k", 3);
      }
      else if(!strcmp("close", (char*)cmd))
      {
         send(0, (uint8_t*)"close", 5);
      }

      //timerCheck(&timer_obj[TIMER_0], HAL_GetTick());
      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
   }
   /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
   RCC_OscInitTypeDef RCC_OscInitStruct =
   {0};
   RCC_ClkInitTypeDef RCC_ClkInitStruct =
   {0};

   /** Configure the main internal regulator output voltage
    */
   __HAL_RCC_PWR_CLK_ENABLE();
   __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

   /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
   RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
   RCC_OscInitStruct.HSEState = RCC_HSE_ON;
   RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
   RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
   RCC_OscInitStruct.PLL.PLLM = 8;
   RCC_OscInitStruct.PLL.PLLN = 336;
   RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
   RCC_OscInitStruct.PLL.PLLQ = 7;
   if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
   {
      Error_Handler();
   }

   /** Initializes the CPU, AHB and APB buses clocks
    */
   RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
   RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
   RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
   RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
   RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

   if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
   {
      Error_Handler();
   }
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void)
{

   /* USER CODE BEGIN SPI2_Init 0 */

   /* USER CODE END SPI2_Init 0 */

   /* USER CODE BEGIN SPI2_Init 1 */

   /* USER CODE END SPI2_Init 1 */
   /* SPI2 parameter configuration*/
   hspi2.Instance = SPI2;
   hspi2.Init.Mode = SPI_MODE_MASTER;
   hspi2.Init.Direction = SPI_DIRECTION_2LINES;
   hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
   hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
   hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
   hspi2.Init.NSS = SPI_NSS_SOFT;
   hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
   hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
   hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
   hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
   hspi2.Init.CRCPolynomial = 10;
   if(HAL_SPI_Init(&hspi2) != HAL_OK)
   {
      Error_Handler();
   }
   /* USER CODE BEGIN SPI2_Init 2 */

   /* USER CODE END SPI2_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

   /* DMA controller clock enable */
   __HAL_RCC_DMA1_CLK_ENABLE();

   /* DMA interrupt init */
   /* DMA1_Stream3_IRQn interrupt configuration */
   HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
   HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
   /* DMA1_Stream4_IRQn interrupt configuration */
   HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
   HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
   GPIO_InitTypeDef GPIO_InitStruct =
   {0};
   /* USER CODE BEGIN MX_GPIO_Init_1 */
   /* USER CODE END MX_GPIO_Init_1 */

   /* GPIO Ports Clock Enable */
   __HAL_RCC_GPIOH_CLK_ENABLE();
   __HAL_RCC_GPIOC_CLK_ENABLE();
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPIOD_CLK_ENABLE();

   /*Configure GPIO pin Output Level */
   HAL_GPIO_WritePin(ETH_CS_GPIO_Port, ETH_CS_Pin, GPIO_PIN_SET);

   /*Configure GPIO pin Output Level */
   HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_RESET);

   /*Configure GPIO pin Output Level */
   HAL_GPIO_WritePin(GPIOD,
   LD_GREEN_Pin | LD_ORANGE_Pin | LD_RED_Pin | LD_BLUE_Pin, GPIO_PIN_RESET);

   /*Configure GPIO pin : BTN_Pin */
   GPIO_InitStruct.Pin = BTN_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   HAL_GPIO_Init(BTN_GPIO_Port, &GPIO_InitStruct);

   /*Configure GPIO pin : ETH_CS_Pin */
   GPIO_InitStruct.Pin = ETH_CS_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(ETH_CS_GPIO_Port, &GPIO_InitStruct);

   /*Configure GPIO pin : ETH_INT_Pin */
   GPIO_InitStruct.Pin = ETH_INT_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   HAL_GPIO_Init(ETH_INT_GPIO_Port, &GPIO_InitStruct);

   /*Configure GPIO pin : ETH_RST_Pin */
   GPIO_InitStruct.Pin = ETH_RST_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(ETH_RST_GPIO_Port, &GPIO_InitStruct);

   /*Configure GPIO pins : LD_GREEN_Pin LD_ORANGE_Pin LD_RED_Pin LD_BLUE_Pin */
   GPIO_InitStruct.Pin =
   LD_GREEN_Pin | LD_ORANGE_Pin | LD_RED_Pin | LD_BLUE_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

   /* EXTI interrupt init*/
   HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 1);
   HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

   /* USER CODE BEGIN MX_GPIO_Init_2 */
   /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void printInfo(void)
{
   print("---------------------\n");
   print("Network configuration:\r\n");
   print("IP ADDRESS:  %d.%d.%d.%d\r\n", app_net_data.net_info.ip[0], app_net_data.net_info.ip[1], app_net_data.net_info.ip[2],
         app_net_data.net_info.ip[3]);
   print("NETMASK:     %d.%d.%d.%d\r\n", app_net_data.net_info.sn[0], app_net_data.net_info.sn[1], app_net_data.net_info.sn[2],
         app_net_data.net_info.sn[3]);
   print("GATEWAY:     %d.%d.%d.%d\r\n", app_net_data.net_info.gw[0], app_net_data.net_info.gw[1], app_net_data.net_info.gw[2],
         app_net_data.net_info.gw[3]);
   print("DNS ADDRESS: %d.%d.%d.%d\r\n", app_net_data.net_info.dns[0], app_net_data.net_info.dns[1], app_net_data.net_info.dns[2],
            app_net_data.net_info.dns[3]);
   print("MAC ADDRESS: %x:%x:%x:%x:%x:%x\r\n", app_net_data.net_info.mac[0], app_net_data.net_info.mac[1], app_net_data.net_info.mac[2],
         app_net_data.net_info.mac[3], app_net_data.net_info.mac[4], app_net_data.net_info.mac[5]);
   print("---------------------\n");
}

static void doSendOk(uint8_t sn, uint16_t len)
{
   print("------------------\n"
         "Socket: %d\n"
         "%d bytes data sent\n"
         "------------------\n", sn, len);
}

static void doTimeout(uint8_t sn, uint8_t discon)
{
   char info[30];
   switch(discon)
   {
   case 0:
      strcpy(info, "timeout");
   break;
   case 1:
      strcpy(info, "timeout by disconnect");
   break;
   case 2:
      strcpy(info, "timeout by connect to host");
   break;
   }
   print("%s\n", info);
}

static void doRecv(uint8_t sn, uint16_t len)
{
   int32_t ret;

   ret = recv(sn, buf, len);
   print("------------------\n"
         "received size: %d\n"
         "ret: %d\n"
         "------------------\n", len, ret);
   //print("Send return: %d\n", send(sn, buf, len));
   send(sn, buf, len);
}

static void doDiscon(uint8_t sn)
{
   print("Socket %d disconnected\n", sn);
}

static void doCon(uint8_t sn)
{
   getSn_DIPR(sn, app_net_data.destip);// client ip adresini al
   app_net_data.destport = getSn_DPORT(sn);
   print("Socket %d:Connected - %d.%d.%d.%d : %d\r\n", sn, app_net_data.destip[0], app_net_data.destip[1], app_net_data.destip[2],
         app_net_data.destip[3], app_net_data.destport);
}

static void doEthfault(uint8_t sn)
{
   print("Ethfault!\n");
}

static void doLinkOff(void)
{
   print("check the eth cable or MOSI-MISO!\n");
}

static void doConflict(void)
{
   print("conflict!\n");
}

static void doUnreach(void)
{
   print("unreach!\n");
}

static void DHCP_Perform(void)
{
   // DHCP Area
   print("DHCP START\n");
   reg_dhcp_cbfunc(NULL, NULL, NULL);
   DHCP_init(app_net_data.sn, app_net_data.dhcp_buf);

   uint8_t dhcpret;
   while((dhcpret = DHCP_run()) == DHCP_RUNNING); // sonsuz döngü değil

   if(dhcpret == DHCP_IP_LEASED)
   {
      print("DHCP Successful\n");
   }
   else if(dhcpret == DHCP_FAILED)
   {
      // ayrıca failed timeout sonucu da oluşur(5 sn ayarladım)
      print("DHCP Failed!\n");
   }

   // at this point, socket is in UDP mode and open, it has been closed
   if(close(app_net_data.sn) != SOCK_OK)
      print("socket cannot close!\n");

   // load the net info
   getIPfromDHCP(app_net_data.net_info.ip);
   getGWfromDHCP(app_net_data.net_info.gw);
   getSNfromDHCP(app_net_data.net_info.sn);
   getDNSfromDHCP(app_net_data.net_info.dns);
   printInfo();
}
static void periodicTimer(void)
{
   ethObserver(app_net_data.sn);
}

void cs_sel(void)
{
   HAL_GPIO_WritePin(ETH_CS_GPIO_Port, ETH_CS_Pin, GPIO_PIN_RESET);//CS LOW
}

void cs_desel(void)
{
   HAL_GPIO_WritePin(ETH_CS_GPIO_Port, ETH_CS_Pin, GPIO_PIN_SET);//CS HIGH
}

static void spi_rb_burst_dma(uint8_t *rbuf, uint16_t len)
{
   HAL_SPI_Receive_DMA(&hspi2, rbuf, len);
//HAL_DMA_PollForTransfer(&hdma_spi2_rx, HAL_DMA_FULL_TRANSFER, 5);
   while (!spi_rx_done);
   spi_rx_done = 0;
}

static void spi_wb_burst_dma(uint8_t *tbuf, uint16_t len)
{
   HAL_SPI_Transmit_DMA(&hspi2, tbuf, len);
//HAL_DMA_PollForTransfer(&hdma_spi2_tx, HAL_DMA_FULL_TRANSFER, 5);
   while (!spi_tx_done);
   spi_tx_done = 0;
}

static void spi_rb_burst(uint8_t *rbuf, uint16_t len)
{
   HAL_SPI_Receive(&hspi2, rbuf, len, 100);
}

static void spi_wb_burst(uint8_t *tbuf, uint16_t len)
{
   HAL_SPI_Transmit(&hspi2, tbuf, len, 100);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
   spi_tx_done = 1;
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
   spi_rx_done = 1;
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
   /* USER CODE BEGIN Error_Handler_Debug */
   /* User can add his own implementation to report the HAL error return state */
   __disable_irq();
   while (1)
   {
   }
   /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
