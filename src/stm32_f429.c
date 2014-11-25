#include "stm32_f429.h"
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_exti.h"
#include "misc.h"

void RCC_Configuration(void)
{
      /* --------------------------- System Clocks Configuration -----------------*/
      /* USART1 clock enable */
      RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
      /* GPIO clock enable */
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
      RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
      /* Timers clock enable */
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
      RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
}


/**************************************************************************************/
 
void GPIOA_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructureA;

    /*-------------------------- GPIO Configuration ----------------------------*/
    GPIO_InitStructureA.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructureA.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructureA.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructureA.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructureA.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructureA);

    /* Connect USART pins to AF */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);   // USART1_TX
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  // USART1_RX
}

void GPIOB_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructureB;
    GPIO_InitStructureB.GPIO_Pin = GPIO_Pin_4; 
    GPIO_InitStructureB.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructureB.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructureB.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructureB.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructureB);

    GPIO_InitStructureB.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15; 
    GPIO_InitStructureB.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructureB.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructureB.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructureB.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructureB);
}

void GPIOC_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructureC;
    GPIO_InitStructureC.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_8 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5; 
    GPIO_InitStructureC.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructureC.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructureC.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructureC.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructureC);
}

void GPIOG_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructureG;
    GPIO_InitStructureG.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_9 | GPIO_Pin_13 | GPIO_Pin_14; 
    GPIO_InitStructureG.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructureG.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructureG.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructureG.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOG, &GPIO_InitStructureG);
}

void GPIOE_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructureE;
    GPIO_InitStructureE.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4; 
    GPIO_InitStructureE.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructureE.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructureE.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructureE.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &GPIO_InitStructureE);
}
 
/**************************************************************************************/
 
void USART1_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;

    /* USARTx configuration ------------------------------------------------------*/
    /* USARTx configured as follow:
     *  - BaudRate = 9600 baud
     *  - Word Length = 8 Bits
     *  - One Stop Bit
     *  - No parity
     *  - Hardware flow control disabled (RTS and CTS signals)
     *  - Receive and transmit enabled
     */
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void init_rs232(void)
{
    RCC_Configuration();
    GPIOA_Configuration();
    USART1_Configuration();
}

void enable_rs232_interrupts(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable transmit and receive interrupts for the USART1. */
    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    /* Enable the USART1 IRQ in the NVIC module (so that the USART1 interrupt
     * handler is enabled). */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void enable_rs232(void)
{
    /* Enable the RS232 port. */
    USART_Cmd(USART1, ENABLE);
}

void TIMER2_Configuration(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    //TIMER 2 is on APB1 -> 84MHz
    TIM_TimeBaseStructure.TIM_Period = 2400 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 10000 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void PWM_Init(void) {
    TIM_OCInitTypeDef TIM_OCStruct;
    
    /* PWM mode 2 = Clear on compare match */
    /* PWM mode 1 = Set on compare match */
    TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
    
/*
    pulse_length = ((TIM_Period + 1) * DutyCycle) / 100 - 1
*/
    TIM_OCStruct.TIM_Pulse = 0;
    TIM_OC1Init(TIM3, &TIM_OCStruct);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_TIM3);
}

void TIMER3_Configuration(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    //TIMER 2 is on APB1 -> 84MHz
    TIM_TimeBaseStructure.TIM_Period = 2400 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 100 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_Cmd(TIM3, ENABLE);
}

