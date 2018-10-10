/********************************************************************
*  File Name: delay.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include <stdint.h>

void delay_us(uint32_t us)
{
    while(us--)
    {
        for(uint32_t i=0;i<8;i++)
        {
            __nop();
        }
    }
}

void delay_ms(uint32_t ms)
{
    while(ms--)
    {
        delay_us(980);
    }
}
