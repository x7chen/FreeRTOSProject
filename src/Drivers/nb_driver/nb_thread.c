/********************************************************************
*  File Name: nb_thread.c
*  Purpose: NB-IOT线程
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include "Driver_USART.h"
#include "includeFreeRTOS.h"
#include <stdio.h>
#include <string.h>
#include "nb_coap.h"
#include "../utils/bit.h"

#define NB_TIMER

void Thread_NB(void *argument);

extern ARM_DRIVER_USART Driver_USART2;
ARM_DRIVER_USART *USARTdrv2 = &Driver_USART2;
EventGroupHandle_t event_nb;
xTaskHandle task_nb;
xTaskHandle task_nb_config;
#define RX_BUFFER_SIZE 64
static uint8_t uart_rx_buff[RX_BUFFER_SIZE];
static uint8_t uart_rx_cache[RX_BUFFER_SIZE];
/*
static char demo[22] = {0x00, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
                     0x05, 0x03, 0x00, 0x09, 0x00, 0x07, 0x02, 0x00,
                     0x08, 0x01, 0x00, 0x02, 0x61, 0x62};
*/

#ifdef NB_TIMER

static TimerHandle_t timer2;
void Timer2Start(uint32_t timerDelay);
static void vTimerCallback(void *arg)
{
    ReceiveDataParser();
}

static void Timer2Create(void)
{
    timer2 = xTimerCreate("nb_timer", 1000, pdFALSE, (void *)1, vTimerCallback);

    if (timer2 == NULL)
    {
    }
}

void Timer2Start(uint32_t timerDelay)
{
    if (xTimerIsTimerActive(timer2))
    {
        xTimerStop(timer2, 5);
    }
    xTimerChangePeriod(timer2, timerDelay, 10);
    if (xTimerStart(timer2, 10) != pdPASS)
    {
        // Timer could not be started
    }
}
#endif //NB_TIMER

void USART2_callback(uint32_t event)
{
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
            xEventGroupSetBits(event_nb, BIT(0));
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

void UART2_init(const void *args)
{
    /*Initialize the USART driver */
    USARTdrv2->Initialize(USART2_callback);
    /*Power up the USART peripheral */
    USARTdrv2->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 9600 Bits/sec */
    USARTdrv2->Control(ARM_USART_MODE_ASYNCHRONOUS |
                           ARM_USART_DATA_BITS_8 |
                           ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 |
                           ARM_USART_FLOW_CONTROL_NONE,
                       9600);

    /* Enable Receiver and Transmitter lines */
    USARTdrv2->Control(ARM_USART_CONTROL_TX, 1);
    USARTdrv2->Control(ARM_USART_CONTROL_RX, 1);
}

int Init_Thread_NB(void)
{
    xTaskCreate(Thread_NB,
                "Task_nb",
                256,
                NULL,
                3,
                &task_nb);

    event_nb = xEventGroupCreate();

    return (0);
}
int Init_Thread_NB_config(void)
{
    xTaskCreate(Init_NB05,
                "Task_nb_config",
                256,
                NULL,
                3,
                &task_nb_config);
    return (0);
}
void Thread_NB(void *args)
{
    int n;
    EventBits_t event_bit;
    UART2_init(NULL);
    Timer2Create();
    Init_Thread_NB_config();
    USARTdrv2->Receive(uart_rx_buff, RX_BUFFER_SIZE);
    while (1)
    {
        //先设置接收长度，然后等待
        event_bit = xEventGroupWaitBits(event_nb, BIT(0), pdTRUE, pdFALSE, 100);
        if (event_bit & BIT(0))
        {
            n = USARTdrv2->GetRxCount();
            if (n > 0)
            {
                memcpy(uart_rx_cache, uart_rx_buff, n);
                USARTdrv2->Receive(uart_rx_buff, RX_BUFFER_SIZE);
                Timer2Start(200); //设置100ms的延时，检测传输完成。
                nb_rx_buffer.append_data(&nb_rx_buffer, uart_rx_cache, n);
            }
        }
        if ((event_bit & BIT(0)) == 0)
        {
            n = USARTdrv2->GetRxCount();
            if (n > 0)
            {
                memcpy(uart_rx_cache, uart_rx_buff, n);
                USARTdrv2->Control(ARM_USART_ABORT_RECEIVE, NULL);
                USARTdrv2->Receive(uart_rx_buff, RX_BUFFER_SIZE);
                Timer2Start(200); //设置100ms的延时，检测传输完成。
                nb_rx_buffer.append_data(&nb_rx_buffer, uart_rx_cache, n);
            }
        }
        lock_control();
    }
}
