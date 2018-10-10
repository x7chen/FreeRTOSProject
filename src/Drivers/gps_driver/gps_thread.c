/********************************************************************
*  File Name: gps_thread.c
*  Purpose: GPS线程
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include "Driver_USART.h"
#include "includeFreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include "gps_parser.h"
#include "../utils/bit.h"
#include "Board_LED.h"

void Thread_GPS(void *argument);

extern ARM_DRIVER_USART Driver_USART4;
ARM_DRIVER_USART *USARTdrv4 = &Driver_USART4;

EventGroupHandle_t event_gps;
xTaskHandle task_gps;
#define RX_BUFFER_SIZE         64
static uint8_t uart_rx_buff[RX_BUFFER_SIZE];
static uint8_t uart_rx_cache[RX_BUFFER_SIZE];
#ifdef GPS_TIMER 
static TimerHandle_t timer4;
static void vTimerCallback(void *arg)
{
    
}

static void Timer4Create(void)
{
    timer4 = xTimerCreate("gps_timer", 1000, pdFALSE, (void *)1, vTimerCallback);

    if (timer4 == NULL)
    {
        
    }
}

static void Timer4Start(uint32_t timerDelay)
{
    if (xTimerIsTimerActive(timer4))
    {
        xTimerStop(timer4, 5);
    }
    xTimerChangePeriod(timer4, timerDelay, 100);
    if (xTimerStart(timer4, 100) != pdPASS)
    {
        // Timer could not be started
    }
}
#endif //GPS_TIMER

void USART4_callback(uint32_t event)
{
    BaseType_t xResult;
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t mask;
    mask = ARM_USART_EVENT_RECEIVE_COMPLETE |
           ARM_USART_EVENT_TRANSFER_COMPLETE |
           ARM_USART_EVENT_SEND_COMPLETE |
           ARM_USART_EVENT_TX_COMPLETE;
    if (event & mask)
    {
        /* Success: Wakeup Thread */
        if (event & ARM_USART_EVENT_RECEIVE_COMPLETE)
        {
            // xResult = xEventGroupSetBitsFromISR(event_gps, BIT(0), &xHigherPriorityTaskWoken);        
            //调用一定次数后有问题用xEventGroupSetBits替换后正常
            xResult = xEventGroupSetBits(event_gps, BIT(0));
            if (xResult != pdFAIL)
            {
                // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
    if (event & ARM_USART_EVENT_RX_TIMEOUT)
    {
        //__breakpoint(0); /* Error: Call debugger or replace with custom error handling */
    }
    if (event & (ARM_USART_EVENT_RX_OVERFLOW | ARM_USART_EVENT_TX_UNDERFLOW))
    {
        // __breakpoint(0); /* Error: Call debugger or replace with custom error handling */
    }
}

void UART4_init(const void *args)
{
    /*Initialize the USART driver */
    USARTdrv4->Initialize(USART4_callback);
    /*Power up the USART peripheral */
    USARTdrv4->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 9600 Bits/sec */
    USARTdrv4->Control(ARM_USART_MODE_ASYNCHRONOUS |
                           ARM_USART_DATA_BITS_8 |
                           ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 |
                           ARM_USART_FLOW_CONTROL_NONE,
                       9600);

    /* Enable Receiver and Transmitter lines */
    USARTdrv4->Control(ARM_USART_CONTROL_TX, 1);
    USARTdrv4->Control(ARM_USART_CONTROL_RX, 1);
}

int Init_Thread_GPS(void)
{
    xTaskCreate(Thread_GPS,         //线程主函数
                "Task_gps",         //线程名称
                256,                //线程栈大小，单位word（4byte）
                NULL,               //传送参数（主函数参数）
                3,                 //优先级
                &task_gps);              //线程Handle传出
    event_gps = xEventGroupCreate();
    return (0);
}

void Thread_GPS(void *args)
{
    uint32_t n=0;
    EventBits_t event_bit;
    UART4_init(NULL);
    GPS_init();
    
    USARTdrv4->Receive(uart_rx_buff, RX_BUFFER_SIZE);
    while (1)
    {
         //先设置接收长度，然后等待
        event_bit = xEventGroupWaitBits(event_gps, BIT(0), pdTRUE, pdFALSE, 1000);
        if (event_bit & BIT(0))
        {
            n = USARTdrv4->GetRxCount();
            if(n > 0)
            {
                memcpy(uart_rx_cache,uart_rx_buff,n);
                USARTdrv4->Receive(uart_rx_buff, RX_BUFFER_SIZE);
                GPS_Receive(uart_rx_cache, n);
            }
        }
        if((event_bit&BIT(0))==0)
        {
            n = USARTdrv4->GetRxCount();
            if(n > 0)
            {
                memcpy(uart_rx_cache,uart_rx_buff,n);
                USARTdrv4->Control(ARM_USART_ABORT_RECEIVE,NULL);
                USARTdrv4->Receive(uart_rx_buff, RX_BUFFER_SIZE);
                GPS_Receive(uart_rx_cache, n);
            }
        }
        if(gps_power)
        {
            PIN_High(GPS_EN);
        }
        else
        {
            PIN_Low(GPS_EN);
        }
    }
}
