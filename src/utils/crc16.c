/********************************************************************
*  File Name: crc16.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/
#include "crc16.h"
#include <stdio.h>

uint16_t crc16_compute(const uint8_t * p_data, uint32_t size, const uint16_t * p_original)
{
    uint32_t i;
    uint16_t crc = (p_original == NULL) ? 0xffff : *p_original;

    for (i = 0; i < size; i++)
    {
        crc  = (unsigned char)(crc >> 8) | (crc << 8);
        crc ^= p_data[i];
        crc ^= (unsigned char)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }

    return crc;
}
