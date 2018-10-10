/********************************************************************
*  File Name: log_thread.c
*  Purpose: 日志线程
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include "Driver_USART.h"
#include "includeFreeRTOS.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "../utils/bit.h"
#include "../utils/bytewise.h"
#include "Board_LED.h"
#include "../drivers/uhf_driver/uhf_protocol.h"
#include "../drivers/nb_driver/nb_coap.h"
#include "log.h"
#include "../nv_data/nv_data.h"
#define RX_BUFFER_SIZE 64
static uint8_t uart_rx_buff[RX_BUFFER_SIZE];
static uint8_t uart_rx_cache[RX_BUFFER_SIZE];
static uint8_t log_rx_data[128];
//static uint8_t uart_tx_buff[LOG_LINE_SIZE_MAX+2];
static log_msg_t log_msg_rx;
static log_msg_t log_msg_tx;
void Thread_LOG(void *argument);

extern ARM_DRIVER_USART Driver_USART1;
ARM_DRIVER_USART *USARTdrv1 = &Driver_USART1;
xTaskHandle task_log;
EventGroupHandle_t event_log;
QueueHandle_t queue_log;

buffer_t log_rx_buffer = BUFFER_DEFAULT;
void CMD_Process(void);

#define LOG_TIMER
#ifdef LOG_TIMER
static TimerHandle_t timer1;
void Timer1Start(uint32_t timerDelay);
static void vTimerCallback(void *arg)
{
    CMD_Process();
}

static void Timer1Create(void)
{
    timer1 = xTimerCreate("log_timer", 1000, pdFALSE, (void *)1, vTimerCallback);

    if (timer1 == NULL)
    {
    }
}

void Timer1Start(uint32_t timerDelay)
{
    if (xTimerIsTimerActive(timer1))
    {
        xTimerStop(timer1, 5);
    }
    xTimerChangePeriod(timer1, timerDelay, 10);
    if (xTimerStart(timer1, 10) != pdPASS)
    {
        // Timer could not be started
    }
}
#endif //LOG_TIMER

void USART1_callback(uint32_t event)
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
            // 调用一定次数后有问题用xEventGroupSetBits替换后正常
            xResult = xEventGroupSetBits(event_log, BIT(0));
            if (xResult != pdFAIL)
            {
                //  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
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

void UART1_init(const void *args)
{
    /*Initialize the USART driver */
    USARTdrv1->Initialize(USART1_callback);
    /*Power up the USART peripheral */
    USARTdrv1->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 9600 Bits/sec */
    USARTdrv1->Control(ARM_USART_MODE_ASYNCHRONOUS |
                           ARM_USART_DATA_BITS_8 |
                           ARM_USART_PARITY_NONE |
                           ARM_USART_STOP_BITS_1 |
                           ARM_USART_FLOW_CONTROL_NONE,
                       115200);

    /* Enable Receiver and Transmitter lines */
    USARTdrv1->Control(ARM_USART_CONTROL_TX, 1);
    USARTdrv1->Control(ARM_USART_CONTROL_RX, 1);
}

void LOG_init()
{
    log_rx_buffer.initialize(&log_rx_buffer,log_rx_data,sizeof(log_rx_data));
}
void CMD_Process(void)
{
    uint32_t box_id;
    if (memcmp(log_rx_buffer.data, "SET ", 4) == 0)
    {
        if (memcmp(log_rx_buffer.data + 4, "BOX_ID=", 7) == 0)
        {
            box_id = strtol((char *)(log_rx_buffer.data + 11), NULL, 10);
            nv_setBoxNum((uint8_t *)&box_id);
            LOG_printf("SET Box_ID = %u seccess.", box_id);
            save_config();
        }
    }
    if (memcmp(log_rx_buffer.data, "GET ", 4) == 0)
    {
        if (memcmp(log_rx_buffer.data + 4, "BOX_ID", 6) == 0)
        {
            nv_getBoxNum((uint8_t *)&box_id);
            LOG_printf("Box_ID = %u", box_id);
        }
    }
    if (memcmp(log_rx_buffer.data, "ECHO ", 5) == 0)
    {
        LOG_Ascii((char *)(log_rx_buffer.data + 5), log_rx_buffer.count - 5);
    }
    log_rx_buffer.clear(&log_rx_buffer);
}
void LOG_Ascii(char *msg, uint16_t length)
{
    uint16_t valid_len = length;
    if (length >= LOG_LINE_SIZE_MAX)
    {
        valid_len = LOG_LINE_SIZE_MAX - 1;
    }
    log_msg_tx.length = valid_len + 1;
    memcpy(log_msg_tx.data, msg, valid_len);
    memcpy(log_msg_tx.data + valid_len, "\n", 1);
    xQueueSend(queue_log, &log_msg_tx, 10);
}

void LOG_Hex(uint8_t *data, uint16_t length)
{
    uint16_t valid_len = length;
    if (length >= LOG_LINE_SIZE_MAX / 2)
    {
        valid_len = LOG_LINE_SIZE_MAX / 2 - 1;
    }
    for (int i = 0; i < valid_len; i++)
    {
        sprintf((char *)(log_msg_tx.data + 2 * i), "%02X", data[i]);
    }
    memcpy(log_msg_tx.data + valid_len * 2, "\n", 1);
    log_msg_tx.length = valid_len * 2 + 1;
    xQueueSend(queue_log, &log_msg_tx, 10);
}

/***************************************
*  LOG_printf
*  实验功能，请谨慎使用(注意控制长度)
****************************************/
void LOG_printf(const char *str, ...)
{
    va_list ap;
    va_start(ap, str);
    vsprintf((char *)(log_msg_tx.data), str, ap);
    sprintf((char *)(log_msg_tx.data + strlen((char *)(log_msg_tx.data))), "\n");
    va_end(ap);
    log_msg_tx.length = strlen((char *)(log_msg_tx.data));
    xQueueSend(queue_log, &log_msg_tx, 10);
}

void LOG_Receive(uint8_t *data, uint16_t length)
{
    log_rx_buffer.append_data(&log_rx_buffer, data, length);
}

int Init_Thread_LOG(void)
{
    xTaskCreate(Thread_LOG, //线程主函数
                "Task_LOG", //线程名称
                256,        //线程栈大小，单位word（4byte）
                NULL,       //传送参数（主函数参数）
                3,          //优先级
                &task_log);      //线程Handle传出
    event_log = xEventGroupCreate();
    queue_log = xQueueCreate(10, sizeof(log_msg_t));
    return (0);
}

void Thread_LOG(void *args)
{
    uint32_t n = 0;
    EventBits_t event_bit;
    UART1_init(NULL);
    LOG_init();
    Timer1Create();
    USARTdrv1->Receive(uart_rx_buff, RX_BUFFER_SIZE);
    while (1)
    {
        //先设置接收长度，然后等待
        event_bit = xEventGroupWaitBits(event_log, BIT(0), pdTRUE, pdFALSE, 10);
        if (event_bit & BIT(0))
        {
            n = USARTdrv1->GetRxCount();
            if (n > 0)
            {
                memcpy(uart_rx_cache, uart_rx_buff, n);
                USARTdrv1->Receive(uart_rx_buff, RX_BUFFER_SIZE);
                Timer1Start(100);
                log_rx_buffer.append_data(&log_rx_buffer, uart_rx_cache, n);
            }
        }
        else
        {
            n = USARTdrv1->GetRxCount();
            if (n > 0)
            {
                memcpy(uart_rx_cache, uart_rx_buff, n);
                USARTdrv1->Control(ARM_USART_ABORT_RECEIVE, NULL);
                USARTdrv1->Receive(uart_rx_buff, RX_BUFFER_SIZE);
                Timer1Start(100);
                log_rx_buffer.append_data(&log_rx_buffer, uart_rx_cache, n);
            }
        }
        if (USARTdrv1->GetStatus().tx_busy == 0)
        {
            if (pdTRUE == xQueueReceive(queue_log, &log_msg_rx, 10))
            {
                USARTdrv1->Send(log_msg_rx.data, log_msg_rx.length);
            }
        }
    }
}
