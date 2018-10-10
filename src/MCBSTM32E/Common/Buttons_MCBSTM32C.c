/********************************************************************
 * File Name: Buttons_MCBSTM32C.c
 * Purpose: 在原bsp代码的基础上增加按键事件注册，长按等功能。
 * Author: Sean
 * Date: 2018-06-28
 *********************************************************************/

#include "Board_Buttons.h"
#include "GPIO_STM32F10x.h"
#include "includeFreeRTOS.h"
#include "../utils/bit.h"
/* Buttons for MCBSTM32C */

const GPIO_PIN_ID Pin_Buttons[] = {
    {GPIOA, 0},
    {GPIOB, 8}};

#define BUTTONS_COUNT (sizeof(Pin_Buttons) / sizeof(GPIO_PIN_ID)) /* Number of available buttons        */

static void Timer6Create(void);
static void bsp_button_event_handler(uint8_t pin_no, uint8_t button_action);
static bsp_event_callback_t m_registered_callback = NULL;
static uint8_t current_long_push_pin_no;
static bsp_button_event_cfg_t m_events_list[BUTTONS_COUNT] = {{BSP_EVENT_NOTHING}, {BSP_EVENT_NOTHING}};
/**
  \fn          int32_t Buttons_Initialize (void)
  \brief       Initialize buttons
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Buttons_Initialize()
{
    uint32_t n;
    /* Configure pins: Push-pull Output Mode (50 MHz) with Pull-down resistors */
    for (n = 0; n < BUTTONS_COUNT; n++)
    {
        GPIO_PortClock(Pin_Buttons[n].port, true);
        GPIO_PinConfigure(Pin_Buttons[n].port, Pin_Buttons[n].num,
                          GPIO_IN_PULL_UP,
                          GPIO_MODE_INPUT);
    }
    Timer6Create();

    return 0;
}

/**
  \fn          int32_t Buttons_Uninitialize (void)
  \brief       De-initialize buttons
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Buttons_Uninitialize(void)
{
    return 0;
}

/**
  \fn          uint32_t Buttons_GetState (void)
  \brief       Get buttons state
  \returns     Buttons state
*/
uint32_t Buttons_GetState(void)
{
    uint32_t val = 0;
    for (int n = 0; n < BUTTONS_COUNT; n++)
    {
        if (GPIO_PinRead(Pin_Buttons[n].port, Pin_Buttons[n].num) == 1)
        {
            val |= BIT(n);
        }
    }
    return (val);
}

/**
  \fn          uint32_t Buttons_GetCount (void)
  \brief       Get number of available buttons
  \return      Number of available buttons
*/
uint32_t Buttons_GetCount(void)
{
    return BUTTONS_COUNT;
}

void register_buttons_event(bsp_event_callback_t callback)
{
    m_registered_callback = callback;
}
static void vTimerCallback(void *arg)
{
    bsp_button_event_handler(current_long_push_pin_no, BSP_BUTTON_ACTION_LONG_PUSH);
}

TimerHandle_t timer6;
static void Timer6Create(void)
{
    timer6 = xTimerCreate("button_timer", 1000, pdFALSE, (void *)1, vTimerCallback);

    if (timer6 == NULL)
    {
    }
}

static void Timer6Start(uint32_t timerDelay)
{
    if (xTimerIsTimerActive(timer6))
    {
        xTimerStop(timer6, 5);
    }
    xTimerChangePeriod(timer6, timerDelay, 10);

    if (xTimerStart(timer6, 10) != pdPASS)
    {
        // Timer could not be started
    }
}

static void Timer6Stop()
{
    xTimerStop(timer6, 10);
}

static void bsp_button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    bsp_event_t event = BSP_EVENT_NOTHING;
    uint32_t button = 0;
    /**< Pin number of a currently pushed button, that could become a long push if held long enough. */
    static bsp_event_t release_event_at_push[BUTTONS_COUNT]; /**< Array of what the release event of each button was last time it was pushed, so that no release event is sent if the event was bound after the push of the button. */

    while ((button < BUTTONS_COUNT) && (button != pin_no))
    {
        button++;
    }

    if (button < BUTTONS_COUNT)
    {
        switch (button_action)
        {
        case APP_BUTTON_PUSH:
            event = m_events_list[button].push_event;
            if (m_events_list[button].long_push_event != BSP_EVENT_NOTHING)
            {
                Timer6Start(1000);
                current_long_push_pin_no = pin_no;
            }
            release_event_at_push[button] = m_events_list[button].release_event;
            break;
        case APP_BUTTON_RELEASE:
            Timer6Stop();
            if (release_event_at_push[button] == m_events_list[button].release_event)
            {
                event = m_events_list[button].release_event;
            }
            break;
        case BSP_BUTTON_ACTION_LONG_PUSH:
            event = m_events_list[button].long_push_event;
        }
    }

    if ((event != BSP_EVENT_NOTHING) && (m_registered_callback != NULL))
    {
        m_registered_callback(event);
    }
}
void bsp_event_to_button_action_assign(uint32_t button, bsp_button_action_t action, bsp_event_t event)
{

    if (button < BUTTONS_COUNT)
    {
        if (event == BSP_EVENT_DEFAULT)
        {
            // Setting default action: BSP_EVENT_KEY_x for PUSH actions, BSP_EVENT_NOTHING for RELEASE and LONG_PUSH actions.
            event = (action == BSP_BUTTON_ACTION_PUSH) ? (bsp_event_t)(BSP_EVENT_KEY_0 + button) : BSP_EVENT_NOTHING;
        }
        switch (action)
        {
        case BSP_BUTTON_ACTION_PUSH:
            m_events_list[button].push_event = event;
            break;
        case BSP_BUTTON_ACTION_LONG_PUSH:
            m_events_list[button].long_push_event = event;
            break;
        case BSP_BUTTON_ACTION_RELEASE:
            m_events_list[button].release_event = event;
            break;
        default:

            break;
        }
    }
}

void buttons_on_event(void)
{
    static uint32_t button_status;
    uint32_t diff_bit, current_status;
    current_status = Buttons_GetState();
    if (button_status != current_status)
    {
        diff_bit = button_status ^ current_status;
        for (uint16_t n = 0; n < BUTTONS_COUNT; n++)
        {
            if (diff_bit & BIT(n))
            {
                bsp_button_event_handler(n, (button_status & BIT(n)) ? APP_BUTTON_PUSH : APP_BUTTON_RELEASE);
            }
        }
    }
    button_status = current_status;
}
