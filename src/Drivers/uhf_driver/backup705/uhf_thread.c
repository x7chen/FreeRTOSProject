/********************************************************************
*  File Name: uhf_thread.c
*  Purpose: UHF线程
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include "Driver_USART.h"
#include "includeFreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include "uhf_protocol.h"
#include "../utils/bit.h"
#include "Board_LED.h"
#include "../Drivers/log_driver/log.h"

EventGroupHandle_t event_uhf;
void Thread_UHF(void *argument);
uint8_t uhf_status = 0 ;
extern ARM_DRIVER_USART Driver_USART3;
ARM_DRIVER_USART *USARTdrv3 = &Driver_USART3; 

#define RX_BUFFER_SIZE         64
static uint8_t uart_rx_buff[RX_BUFFER_SIZE];
static uint8_t uart_rx_cache[RX_BUFFER_SIZE];

#ifdef UHF_TIMER

static TimerHandle_t timer3;
static void vTimerCallback(void *arg)
{
    //uhf_EPC_read();
}

static void Timer3Create(void)
{
    timer3 = xTimerCreate("uhf_timer", 1000, pdTRUE, (void *)1, vTimerCallback);
    if (timer3 != NULL)
    {
    }

}
static void Timer3Start(uint32_t timerDelay)
{
    xTimerChangePeriod(timer3, timerDelay, 10);
    if (xTimerStart(timer3, 10) != pdPASS)
    {
        // Timer could not be started
    }
}

#endif

static void USART3_callback(uint32_t event)
{
    uint32_t mask;
    mask = ARM_USART_EVENT_RECEIVE_COMPLETE |
           ARM_USART_EVENT_TRANSFER_COMPLETE |
           ARM_USART_EVENT_SEND_COMPLETE |
           ARM_USART_EVENT_TX_COMPLETE;
    if (event & mask)
    {
        /* Success: Wakeup Thread */
        if(mask & ARM_USART_EVENT_RECEIVE_COMPLETE)
        {
            xEventGroupSetBits(event_uhf, BIT(0));
        }
    }
    if (event & ARM_USART_EVENT_RX_TIMEOUT)
    {
        //__breakpoint(0); /* Error: Call debugger or replace with custom error handling */
    }
    if (event & (ARM_USART_EVENT_RX_OVERFLOW | ARM_USART_EVENT_TX_UNDERFLOW))
    {
        //__breakpoint(0); /* Error: Call debugger or replace with custom error handling */
    }
}

/* CMSIS-RTOS Thread - UART command thread */
static void UART3_init()
{
    /*Initialize the USART driver */
    USARTdrv3->Initialize(USART3_callback);
    /*Power up the USART peripheral */
    USARTdrv3->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 9600 Bits/sec */
    USARTdrv3->Control(ARM_USART_MODE_ASYNCHRONOUS |
                           ARM_USART_DATA_BITS_8 |
                           ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 |
                           ARM_USART_FLOW_CONTROL_NONE,
                       115200);

    /* Enable Receiver and Transmitter lines */
    USARTdrv3->Control(ARM_USART_CONTROL_TX, 1);
    USARTdrv3->Control(ARM_USART_CONTROL_RX, 1);
    
}
static void UHF_init()
{
    PIN_High(UHF_REST);         /* uhf reset 高正常工作             */
    uhf_protocol_init();
}
int Init_Thread_UHF(void)
{

    xTaskCreate(Thread_UHF,
                "Task_uhf",
                512,
                NULL,
                2,
                NULL);
    event_uhf = xEventGroupCreate();
    xEventGroupClearBits(event_uhf,BIT(0) | BIT(1) | BIT(2));
    return (0);
}


static void Thread_UHF(void *args)
{
    int n;
    EventBits_t event_bit;
    UART3_init();
    UHF_init();
    uhf_protocol_init();
    USARTdrv3->Receive(uart_rx_buff, RX_BUFFER_SIZE);
    while (1)
    {
        
        event_bit = xEventGroupWaitBits(event_uhf, BIT(0) | BIT(1) | BIT(2), pdTRUE, pdFALSE, 100);
        if (event_bit & BIT(0))
        {
            n= USARTdrv3->GetRxCount();                                 //获取已接收字节数
            if(n > 0)
            {
                memcpy(uart_rx_cache,uart_rx_buff, n);
                USARTdrv3->Receive(uart_rx_buff, RX_BUFFER_SIZE);                   //继续接收数据
                uhf_data_receive(uart_rx_cache, n);                     //处理数据
            }
        }
        if((event_bit&(BIT(0) | BIT(1) | BIT(2)))==0)
        {
            n= USARTdrv3->GetRxCount();                                 //获取已接收字节数
            if(n > 0)
            {
                memcpy(uart_rx_cache,uart_rx_buff, n);
                USARTdrv3->Control(ARM_USART_ABORT_RECEIVE,NULL);       //清空DMA计数
                USARTdrv3->Receive(uart_rx_buff, RX_BUFFER_SIZE);          //继续接收
                uhf_data_receive(uart_rx_cache, n);                      //处理数据
                
                           
            }
        }
        if (event_bit & BIT(1))
        {
            
        }
        if (event_bit & BIT(2))     //先开机
        {
            PIN_High(UHF_EN);
            PIN_High(UHF_EN2);
            vTaskDelay(500);
            USARTdrv3->Send(getSendPacket()->data, getSendPacket()->length);
        }
    }
}
