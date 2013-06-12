#include <stdio.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"

#include "pps.h"
#include "usbd_cdc_vcp.h"
#include "uart.h"

static __IO uint32_t LastPPSTime;
static __IO uint8_t PendingPPSTime = 0;

void mainloop_pps() {
}

static uint32_t start_usb;
static uint16_t pending_usb_time = 0;
static uint16_t last_usb_time = 0;
void before_usb_poll() {

  if(PendingPPSTime) {
    start_usb = TIM_GetCounter(TIM2);
    VCP_send_buffer((uint8_t *)"P", 1);
    last_usb_time = pending_usb_time;
    pending_usb_time = 1;
    PendingPPSTime = 0;
  } else {
    pending_usb_time++;
    if(pending_usb_time == 1) { // wrap
      pending_usb_time = 2;
    }
  }
}

void after_usb_poll() {
  static uint32_t pps_before_that = 0;

  if(pending_usb_time == 1) {
    int32_t diff = TIM_GetCounter(TIM2) - start_usb;
    printf(" %lu %lu %ld %ld %u\n",LastPPSTime,start_usb,LastPPSTime-pps_before_that, diff, last_usb_time);
    pps_before_that = LastPPSTime;
  }
}

uint8_t clear_to_print() {
  return pending_usb_time < 495 || pending_usb_time > 505;
}

void EXTI1_IRQHandler(void) {
  if(EXTI_GetITStatus(EXTI_Line1) != RESET) {
    LastPPSTime = TIM_GetCounter(TIM2);
    PendingPPSTime = 1;
    // flash LED
    GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
    // Clear the EXTI line 1 pending bit
    EXTI_ClearITPendingBit(EXTI_Line1);
  }
}

void PPS_init() {
  EXTI_InitTypeDef EXTI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // syscfg for EXTI
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  // connect PA1 to EXTI line1
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource1);
  // Configure EXTI Line1 
  EXTI_InitStructure.EXTI_Line = EXTI_Line1;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}
