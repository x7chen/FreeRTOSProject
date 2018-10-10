/********************************************************************
*  File Name: main.c
*  Purpose: 程序入口，FreeRTOS移植函数实现
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include "includeFreeRTOS.h" // ARM.FreeRTOS::RTOS:Core
#include "app.h"
#include "STM32F10x.h"
void IWDG_Configuration(void);

int main(void)
{
    app_init();
    IWDG_Configuration();           // 看门狗5秒
    vTaskStartScheduler();
    main_loop();
}

void IWDG_Configuration(void)
{
    //写入0x5555,用于允许看门狗寄存器写入功能
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    //看门狗时钟分频,40K/256=156HZ(6.4ms)
    IWDG_SetPrescaler(IWDG_Prescaler_256);

    //喂狗时间 5s/6.4MS=781 .注意不能大于0xfff
    IWDG_SetReload(781);

    //喂狗
    IWDG_ReloadCounter();

    //使能看门狗
    IWDG_Enable();
}

/* FreeRTOS tick timer interrupt handler prototype */
extern void xPortSysTickHandler(void);

/*
  SysTick handler implementation that also clears overflow flag.
*/
void SysTick_Handler(void)
{
    /* Clear overflow flag */
    SysTick->CTRL;

    /* Call tick handler */
    xPortSysTickHandler();
}

/*---------------------------------------------------------------------------*/

/* Callback function prototypes */
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void vApplicationDaemonTaskStartupHook(void);
extern void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

/**
  Dummy implementation of the callback function vApplicationIdleHook().
*/
#if (configUSE_IDLE_HOOK == 1)
__WEAK void vApplicationIdleHook(void)
{
}
#endif

/**
  Dummy implementation of the callback function vApplicationTickHook().
*/
#if (configUSE_TICK_HOOK == 1)
__WEAK void vApplicationTickHook(void)
{
}
#endif

/**
  Dummy implementation of the callback function vApplicationMallocFailedHook().
*/
#if (configUSE_MALLOC_FAILED_HOOK == 1)
__WEAK void vApplicationMallocFailedHook(void)
{
}
#endif

/**
  Dummy implementation of the callback function vApplicationDaemonTaskStartupHook().
*/
#if (configUSE_DAEMON_TASK_STARTUP_HOOK == 1)
__WEAK void vApplicationDaemonTaskStartupHook(void)
{
}
#endif

/**
  Dummy implementation of the callback function vApplicationStackOverflowHook().
*/
#if (configCHECK_FOR_STACK_OVERFLOW > 0)
__WEAK void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
}
#endif

/*---------------------------------------------------------------------------*/

/* External Idle and Timer task static memory allocation functions */
extern void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);
extern void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize);

/* Idle task control block and stack */
static StaticTask_t Idle_TCB;
static StackType_t Idle_Stack[configMINIMAL_STACK_SIZE];

/* Timer task control block and stack */
static StaticTask_t Timer_TCB;
static StackType_t Timer_Stack[configTIMER_TASK_STACK_DEPTH];

/*
  vApplicationGetIdleTaskMemory gets called when configSUPPORT_STATIC_ALLOCATION
  equals to 1 and is required for static memory allocation support.
*/
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &Idle_TCB;
    *ppxIdleTaskStackBuffer = &Idle_Stack[0];
    *pulIdleTaskStackSize = (uint32_t)configMINIMAL_STACK_SIZE;
}

/*
  vApplicationGetTimerTaskMemory gets called when configSUPPORT_STATIC_ALLOCATION
  equals to 1 and is required for static memory allocation support.
*/
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &Timer_TCB;
    *ppxTimerTaskStackBuffer = &Timer_Stack[0];
    *pulTimerTaskStackSize = (uint32_t)configTIMER_TASK_STACK_DEPTH;
}
