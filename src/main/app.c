/********************************************************************
*  File Name: app.c
*  Purpose: 系统初始化，时钟Timer事件，button事件
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/
#include <stdio.h>
#include "stm32f10x.h"       // Device header
#include "includeFreeRTOS.h" // ARM.FreeRTOS::RTOS:Core
#include "Board_LED.h"       // ::Board Support:LED
#include "Board_Buttons.h"   // ::Board Support:LED
#include "Board_ADC.h"       // ::Board Support:A/D Converter
#include "RTE_Components.h"  // Component selection
#include "../utils/clock/clock.h"
#include "../utils/delay.h"
#include "../utils/bit.h"
#include "../Drivers/log_driver/log.h"
#include "../nv_data/nv_data.h"
#include "../Drivers/uhf_driver/uhf_protocol.h"
#include "../Drivers/nb_driver/nb_coap.h"
#include "../Drivers/gps_driver/gps_parser.h"

extern int Init_Thread_LOG(void);
extern int Init_Thread_UHF(void);
extern int Init_Thread_NB(void);
extern int Init_Thread_GPS(void);
extern int Init_Thread_BT(void);
extern EventGroupHandle_t event_log;
extern EventGroupHandle_t event_nb;
extern EventGroupHandle_t event_uhf;
extern EventGroupHandle_t event_gps;
extern EventGroupHandle_t event_bt;

extern xTaskHandle task_log;
extern xTaskHandle task_nb;
extern xTaskHandle task_uhf;
extern xTaskHandle task_gps;
extern xTaskHandle task_bt;

extern uint8_t lock_status;
UTCTimeStruct *time;
clock_callbacks_t clock_callbacks;

char task_list[512];
void print_tasklist()
{
    vTaskList(task_list); //获取task列表
    LOG_Ascii(task_list, 162);
    LOG_Ascii(task_list + 162, 162);
}
void print_task_count()
{
    LOG_printf("Task_Count:%d", uxTaskGetNumberOfTasks()); //获取task数目
}

// 以下信息用于查看堆栈分配是否合理
xTaskHandle task_timer;
void print_heap_stack()
{
    if (task_timer == NULL)
    {
        task_timer = xTimerGetTimerDaemonTaskHandle();
    }
    LOG_printf("Task_log:\t%4d\nTask_nb:\t%4d\nTask_uhf:\t%4d\nTask_gps:\t%4d\nTask_bt:\t%4d\nTask_timer:\t%4d\nHeapFree:\t%4d",
               uxTaskGetStackHighWaterMark(task_log),
               uxTaskGetStackHighWaterMark(task_nb),
               uxTaskGetStackHighWaterMark(task_uhf),
               uxTaskGetStackHighWaterMark(task_gps),
               uxTaskGetStackHighWaterMark(task_bt),
               uxTaskGetStackHighWaterMark(task_timer),
               xPortGetMinimumEverFreeHeapSize());
}

void print_time()
{
    time = get_clock_time();
    LOG_printf("%d-%02d-%02d,%02d:%02d:%02d",
               time->year,
               time->month,
               time->day,
               time->hour,
               time->minutes,
               time->seconds);
}

uint8_t heart_beat_hold = 0;
static void on_second(uint32_t second)
{
    IWDG_ReloadCounter();       //喂狗
    if (second % 5 == 0)
    {
        print_time();
        // print_heap_stack();
        // LOG_printf("ADC:%d",ADC1->DR);
    }
    if(uhf_read_period != 0)
    {
        if (second > 60 && second % (uhf_read_period*10UL) == 0)
        {
            ADC_Start();
        }

        if (second > 60 && second % (uhf_read_period*10UL) == 2)
        {
            xEventGroupSetBits(event_uhf, BIT(1)); //带vTaskDelay()的函数都不能放到timer里面执行
            heart_beat_hold = 1;
        }
    }
    
    if(second > 60 && second % 30 == 7)     //每30秒发送一次心跳包
    {
        if(heart_beat_hold == 0)
        {
            heart_beat_post();
        }
        heart_beat_hold = 0;
    }
    if (gps_power)
    {
        if (second > 60 && second % gps_post_period == 1)
        {
//            if(GPS.status != 0)
            {
                box_location_post();
                heart_beat_hold = 1;
            }
        }
    }
}

static void on_minute(uint32_t minute)
{
    if (minute == 1)
    {
        calibrate_time();
    }
}

static void on_day(uint32_t day)
{
    if (day > 1)
    {
        calibrate_time();
    }
}
TimerHandle_t timer;
static void vTimerCallback(xTimerHandle pxTimer)    // 10ms
{
    static uint64_t count = 0;
    count++;
    buttons_on_event();
    // static uint8_t v = 1;
    if (count > 100 && count % 20 == 0)
    {
        LED_Toggle(LED_0);
        // LED_SetOut(v);   //流水灯
        // v == (1 << 6) ? v = 1 : (v <<= 1);
    }
    if (count % 100 == 0)
    {
        update_clock_second();
    }
}

static void TimerStart(void)
{
    timer = xTimerCreate("clock", 10, pdTRUE, (void *)1, vTimerCallback); //10ms周期

    if (timer != NULL)
    {
        if (xTimerStart(timer, 10) == pdPASS)
        {
            // Timer had be started
        }
    }
}

static void clock_init()
{
    clock_callbacks.on_second_update = on_second;
    clock_callbacks.on_minute_update = on_minute;
    clock_callbacks.on_day_update = on_day;
    register_clock_callbacks(&clock_callbacks);
    TimerStart();
}

static void bsp_event_handler(bsp_event_t evt)
{
    switch (evt)
    {
    case BSP_EVENT_BOX_CLOSE:
        LED_Toggle(LED_1);
        lock_status = 1;
        break;
    case BSP_EVENT_BOX_OPEN:
        LED_Toggle(LED_1);
        lock_status = 0;
        break;
    case BSP_EVENT_KEY_1:
        LED_Toggle(LED_2);
        break;
    case BSP_EVENT_KEY_2:
        LED_Toggle(LED_BT);
        break;
    default:

        break;
    }
}

static void buttons_init()
{
    Buttons_Initialize();
    register_buttons_event(bsp_event_handler); //注册按键事件回调函数

    //绑定按键事件
    bsp_event_to_button_action_assign(0, BSP_BUTTON_ACTION_PUSH, BSP_EVENT_BOX_CLOSE);
    bsp_event_to_button_action_assign(0, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_BOX_OPEN);

    bsp_event_to_button_action_assign(1, BSP_BUTTON_ACTION_RELEASE, BSP_EVENT_KEY_1);
    bsp_event_to_button_action_assign(1, BSP_BUTTON_ACTION_LONG_PUSH, BSP_EVENT_KEY_2);
}

void app_init(void)
{
    load_config();
    LED_Initialize(); /* Initialize LED                 */
    LED_On(LED_0);
    buttons_init();
    ADC_Initialize();
    Init_Thread_LOG();
    Init_Thread_NB();
    Init_Thread_UHF();
    Init_Thread_GPS();
    Init_Thread_BT();

    clock_init();
    //PIN_High(BP_EN); //外部模块 总电源开关
}

void main_loop()
{
    while (1)
    {
        //如果运行到这里说明任务创建失败
        vTaskDelay(100);
    }
}
