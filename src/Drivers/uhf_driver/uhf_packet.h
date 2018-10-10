/********************************************************************
*  File Name: 
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#ifndef _UHF_PACKET_H_
#define _UHF_PACKET_H_
#include "stdint.h"
#include "../../utils/buffer.h"
/*************************************************
*   |-------------------------------------|
*	| SC | TP | CM |  PL   | PB |    CRC  |  
*	|-------------------------------------|
*	| AA | 00 | 07 | 00 01 | 01 | 09 | 10 |
*	|-------------------------------------|
*	|-L1H|---L2H---|---VELUE----|---CRC---|
*        |----Check Sum Range---|
*   
*   SC: Start Code
*   TP: Type        
*   CM: Command
*   PL: Parameter Length
*   PB: Parameter Body
*   CRC: crc16(8005)
*************************************************/

#define START_CODE					0xAA
#define END_CODE					0x00
#define L1_HEADER_VERSION			(0)
#define PACKET_TYPE_NOMAL			0x00
#define PACKET_TYPE_RESP			0x01
#define PACKET_TYPE_NOTIFY			0x02
#define PACKET_TYPE_ACK			    0x08

#define L1_HEADER_OFFSET			(0)
#define L1_HEADER_LENGTH			(1)
#define L2_HEADER_OFFSET			(L1_HEADER_LENGTH)
#define L2_HEADER_LENGTH			(2)
#define L1L2_HEADER_LENGTH			(L1_HEADER_LENGTH+L2_HEADER_LENGTH)
#define VALUE_LEN_OFFSET			(L2_HEADER_OFFSET+L2_HEADER_LENGTH)
#define VALUE_LEN_LENGTH            (2)
#define HEADERS_LENGTH              (L1L2_HEADER_LENGTH+VALUE_LEN_LENGTH)

#define PACKET_STATE_L1HEADER		(1<<7)
#define PACKET_STATE_L2HEADER		(1<<6)
#define PACKET_STATE_VALUE_CNT		(0)
#define PACKET_STATE_VALUE_CNT_MASK (0x1F)

#define MAX_PACKET_LENGTH           256

#define UHF_PACKET_ERROR_BASE       0x00020000UL

#define PACKET_CHECK_SUCCESS        (UHF_PACKET_ERROR_BASE + 0)
#define PACKET_OVER_MAXLENGTH       (UHF_PACKET_ERROR_BASE + 1)
#define PACKET_INVALID              (UHF_PACKET_ERROR_BASE + 2)
#define PACKET_NOT_FINISH           (UHF_PACKET_ERROR_BASE + 3)
#define PACKET_LENGTH_ERROR         (UHF_PACKET_ERROR_BASE + 4)
#define PACKET_OVER_LENGTH          (UHF_PACKET_ERROR_BASE + 5)
#define PACKET_CHECK_FAULT          (UHF_PACKET_ERROR_BASE + 6)


typedef struct 
{
	uint8_t start_code;
	
}Packet_L1_Header_t;
typedef struct 
{
	uint8_t crc16[2];
}Packet_L1_Tail_t;
typedef struct 
{
	uint8_t type;
    uint8_t command;
}Packet_L2_Header_t;

typedef struct 
{
	uint16_t    length;
	uint8_t     *data;
}Packet_Value_t;
typedef struct 
{
	uint8_t     pid;
    uint16_t    length;
	uint8_t     *data;
}Packet_Optional_Value_t;

typedef buffer_t Packet_t;
void packetInit(Packet_t *packet,uint8_t *buff,uint16_t length);
void packetFormat(Packet_t *packet);
void packetClear(Packet_t *packet);
void setL1Header(Packet_t *packet,Packet_L1_Header_t *l1Header);
Packet_L1_Header_t *getL1Header(Packet_t *packet);
void setL2Header(Packet_t *packet,Packet_L2_Header_t *l2Header);
void appendValue(Packet_t *packet,Packet_Value_t *value);
void genL1Header(Packet_t *packet,uint8_t type);

void appendData(Packet_t *packet,uint8_t *data,uint16_t length);
uint32_t packetCheck(Packet_t *packet);

#endif
