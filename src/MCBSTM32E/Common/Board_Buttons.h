/********************************************************************
 * File Name: 
 * Purpose:
 * Author: Sean
 * Date: 2018-06-28
 *********************************************************************/

#ifndef __BOARD_BUTTONS_H
#define __BOARD_BUTTONS_H

#include <stdint.h>
#include "GPIO_STM32F10x.h"
/**
  \fn          int32_t Buttons_Initialize (void)
  \brief       Initialize buttons
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
/**
  \fn          int32_t Buttons_Uninitialize (void)
  \brief       De-initialize buttons
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
/**
  \fn          uint32_t Buttons_GetState (void)
  \brief       Get buttons state
  \returns     Buttons state
*/
/**
  \fn          uint32_t Buttons_GetCount (void)
  \brief       Get number of available buttons
  \return      Number of available buttons
*/

typedef enum
{
    BSP_EVENT_NOTHING = 0, /**< Assign this event to an action to prevent the action from generating an event (disable the action). */
    BSP_EVENT_DEFAULT,     /**< Assign this event to an action to assign the default event to the action. */
    BSP_EVENT_BOX_OPEN,
    BSP_EVENT_BOX_CLOSE,
    BSP_EVENT_KEY_0, /**< Default event of the push action of BSP_BUTTON_0 (only if this button is present). */
    BSP_EVENT_KEY_1, /**< Default event of the push action of BSP_BUTTON_1 (only if this button is present). */
    BSP_EVENT_KEY_2, /**< Default event of the push action of BSP_BUTTON_2 (only if this button is present). */
    BSP_EVENT_KEY_3, /**< Default event of the push action of BSP_BUTTON_3 (only if this button is present). */
    BSP_EVENT_KEY_4, /**< Default event of the push action of BSP_BUTTON_4 (only if this button is present). */
    BSP_EVENT_KEY_5, /**< Default event of the push action of BSP_BUTTON_5 (only if this button is present). */
    BSP_EVENT_KEY_6, /**< Default event of the push action of BSP_BUTTON_6 (only if this button is present). */
    BSP_EVENT_KEY_7, /**< Default event of the push action of BSP_BUTTON_7 (only if this button is present). */
} bsp_event_t;

typedef struct
{
    bsp_event_t push_event;      /**< The event to fire on regular button press. */
    bsp_event_t long_push_event; /**< The event to fire on long button press. */
    bsp_event_t release_event;   /**< The event to fire on button release. */
} bsp_button_event_cfg_t;

typedef void (*app_button_handler_t)(uint8_t pin_no, uint8_t button_action);
typedef void (*bsp_event_callback_t)(bsp_event_t);
typedef uint8_t bsp_button_action_t;

#define APP_BUTTON_PUSH 1        /**< Indicates that a button is pushed. */
#define APP_BUTTON_RELEASE 0     /**< Indicates that a button is released. */
#define APP_BUTTON_ACTIVE_HIGH 1 /**< Indicates that a button is active high. */
#define APP_BUTTON_ACTIVE_LOW 0  /**< Indicates that a button is active low. */

#define BSP_BUTTON_ACTION_PUSH (APP_BUTTON_PUSH)       /**< Represents pushing a button. See @ref bsp_button_action_t. */
#define BSP_BUTTON_ACTION_RELEASE (APP_BUTTON_RELEASE) /**< Represents releasing a button. See @ref bsp_button_action_t. */
#define BSP_BUTTON_ACTION_LONG_PUSH (2)                /**< Represents pushing and holding a button for @ref BSP_LONG_PUSH_TIMEOUT_MS milliseconds. See also @ref bsp_button_action_t. */
extern int32_t Buttons_Initialize(void);
extern int32_t Buttons_Uninitialize(void);
extern uint32_t Buttons_GetState(void);
extern uint32_t Buttons_GetCount(void);
extern void buttons_on_event(void);
extern void register_buttons_event(bsp_event_callback_t event);
extern void bsp_event_to_button_action_assign(uint32_t button, bsp_button_action_t action, bsp_event_t event);

#endif /* __BOARD_BUTTONS_H */
