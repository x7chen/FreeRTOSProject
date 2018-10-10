/********************************************************************
*  File Name: log.h
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#ifndef _LOG_H_
#define _LOG_H_
#include <stdint.h>
#include "../../utils/buffer.h"
#define LOG_LINE_SIZE_MAX 192
typedef struct
{
    uint16_t length;
    uint8_t data[LOG_LINE_SIZE_MAX];
} log_msg_t;

void LOG_Ascii(char *msg, uint16_t length);
void LOG_Hex(uint8_t *data, uint16_t length);
void LOG_printf(const char *str, ...);

#endif
