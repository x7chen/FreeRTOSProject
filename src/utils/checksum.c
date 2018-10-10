/********************************************************************
*  File Name: checksum.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include <stdint.h>
#include "checksum.h"

uint8_t checksum(uint8_t *data,uint16_t len)
{
    uint16_t i;
    uint8_t retval = 0;
    
    for(i = 0; i < len; i++)
    {
        retval += data[i];     
    }
    
    return retval;    
}
