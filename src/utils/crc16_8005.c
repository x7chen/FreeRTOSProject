/********************************************************************
*  File Name: crc16_8005.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/
/***** crc16.c *****/
#include <stdio.h>
#include <stdint.h>
#define CRC16_DNP 0x3D65u   // DNP, IEC 870, M-BUS, wM-BUS, ...
#define CRC16_CCITT 0x1021u // X.25, V.41, HDLC FCS, Bluetooth, ...

//Other polynoms not tested
#define CRC16_IBM 0x8005u     // ModBus, USB, Bisync, CRC-16, CRC-16-ANSI, ...
#define CRC16_T10_DIF 0x8BB7u // SCSI DIF
#define CRC16_DECT 0x0589u    // Cordeless Telephones
#define CRC16_ARINC 0xA02Bu   // ACARS Aplications

#define POLYNOM CRC16_IBM // Define the used polynom from one of the aboves

// Calculates the new crc16 with the newByte. Variable crcValue is the actual or initial value (0).
static unsigned short crc16(unsigned short crcValue, unsigned char newByte)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        if (((crcValue & 0x8000u) >> 8) ^ (newByte & 0x80u))
        {
            crcValue = (crcValue << 1) ^ POLYNOM;
        }
        else
        {
            crcValue = (crcValue << 1);
        }
        newByte <<= 1;
    }
    return crcValue;
}
uint16_t crc16_compute(const uint8_t *p_data, uint32_t size, const uint16_t *original)
{

    unsigned short crc;
    int aux = 0;

    crc = 0x0000u; //Initialization of crc to 0x0000 for DNP
    //crc = 0xFFFFu; //Initialization of crc to 0xFFFF for CCITT

    while (aux < size)
    {
        crc = crc16(crc, p_data[aux]);
        aux++;
    }

    //return (~crc); //The crc value for DNP it is obtained by NOT operation

    return crc; //The crc value for CCITT
}
