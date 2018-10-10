/********************************************************************
*  File Name: bt_thread.c
*  Purpose: 蓝牙线程
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include "Driver_USART.h"
#include "includeFreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include "../utils/bit.h"
#include "Board_LED.h"
#include "../drivers/uhf_driver/uhf_protocol.h"

void Thread_BT(void *argument);
static uint8_t uart_rx_buff[100];
extern ARM_DRIVER_USART Driver_USART5;
ARM_DRIVER_USART *USARTdrv5 = &Driver_USART5;
EventGroupHandle_t event_bt;
xTaskHandle task_bt;
#ifdef BT_TIMER
static TimerHandle_t timer5;

static void vTimerCallback(void *arg)
{
    USARTdrv5->Send(&(uhf_tags.count),2);
    //USARTdrv5->Send(&(uhf_tags.tags[1]),12);
}

static void Timer5Create(void)
{
    timer5 = xTimerCreate("bt_timer", 1000, pdTRUE, (void *)1, vTimerCallback);

    if (timer5 == NULL)
    {
    }
}

staic void Timer5Start(uint32_t timerDelay)
{
    xTimerChangePeriod(timer5, timerDelay, 1000);
    if (xTimerStart(timer5, 100) != pdPASS)
    {
        // Timer could not be started
    }
}
#endif  //BT_TIMER

static void USART5_callback(uint32_t event)
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
//            xResult = xEventGroupSetBitsFromISR(event_gps, BIT(0), &xHigherPriorityTaskWoken);        //调用一定次数后有问题用xEventGroupSetBits替换后正常
            xResult = xEventGroupSetBits(event_bt, BIT(0));
            if (xResult != pdFAIL)
            {
//                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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

static void UART5_init(const void *args)
{
    /*Initialize the USART driver */
    USARTdrv5->Initialize(USART5_callback);
    /*Power up the USART peripheral */
    USARTdrv5->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 9600 Bits/sec */
    USARTdrv5->Control(ARM_USART_MODE_ASYNCHRONOUS |
                           ARM_USART_DATA_BITS_8 |
                           ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 |
                           ARM_USART_FLOW_CONTROL_NONE,
                       9600);

    /* Enable Receiver and Transmitter lines */
    USARTdrv5->Control(ARM_USART_CONTROL_TX, 1);
    USARTdrv5->Control(ARM_USART_CONTROL_RX, 1);
}

void BT_init()
{
    PIN_High(BLE_EN);
    PIN_Low(BLE_CTL);
    PIN_High(BLE_MODE);
    PIN_High(BLE_REST);
}

void BT_Receive(uint8_t *data,uint16_t length)
{
    
}

int Init_Thread_BT(void)
{
    xTaskCreate(Thread_BT,          //线程主函数
                "Task_BT",          //线程名称
                256,                //线程栈大小，单位word（4byte）
                NULL,               //传送参数（主函数参数）
                3,                  //优先级
                &task_bt);              //线程Handle传出
    event_bt = xEventGroupCreate();
    return (0);
}

static void Thread_BT(void *args)
{
    uint32_t n = 0;
    EventBits_t event_bit;
    UART5_init(NULL);
    BT_init();
    while (1)
    {
        USARTdrv5->Receive(uart_rx_buff, 64);
         //先设置接收长度，然后等待
        event_bit = xEventGroupWaitBits(event_bt, BIT(0), pdTRUE, pdFALSE, 50);
        if (event_bit & BIT(0))
        {
            n = USARTdrv5->GetRxCount();
            if(n > 0)
            {
                BT_Receive(uart_rx_buff, n);
            }
            
        }
        else
        {
            n = USARTdrv5->GetRxCount();
            if(n > 0)
            {
                USARTdrv5->Control(ARM_USART_ABORT_RECEIVE,NULL);
                BT_Receive(uart_rx_buff, n);
            }
        }
    }
}
