/********************************************************************
*  File Name: uhf_packet.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include <stdint.h>
#include <string.h>
#include "uhf_packet.h"
#include "../../utils/bytewise.h"
#include "../../utils/crc16_8005.h"

void packetInit(Packet_t *packet, uint8_t *buff,uint16_t length)
{
    packet->initialize(packet,buff,length);
}

void packetFormat(Packet_t *packet)
{
    packet->count = HEADERS_LENGTH;
}
void packetClear(Packet_t *packet)
{
    packet->clear(packet);
}

void setL1Header(Packet_t *packet, Packet_L1_Header_t *l1Header)
{
    packet->set_data(packet,L1_HEADER_OFFSET,(uint8_t *)l1Header,L1_HEADER_LENGTH);
}

void setL2Header(Packet_t *packet, Packet_L2_Header_t *l2Header)
{
    packet->set_data(packet,L2_HEADER_OFFSET,(uint8_t *)l2Header,L2_HEADER_LENGTH);
}

void appendData(Packet_t *packet, uint8_t *data, uint16_t length)
{ 
    packet->append_data(packet,data,length);
}

void setData(Packet_t *packet, uint16_t offset, uint8_t *data, uint16_t length)
{
    packet->set_data(packet,offset,data,length);
}

void genL1Header(Packet_t *packet, uint8_t type)
{
    uint16_t len = Tranverse16(packet->count - HEADERS_LENGTH);
    Packet_L1_Header_t l1Header;
    uint16_t crc16_value;
    l1Header.start_code = START_CODE;
    setL1Header(packet, &l1Header);
    setData(packet, VALUE_LEN_OFFSET, (uint8_t *)&len, VALUE_LEN_LENGTH);
    crc16_value = crc16_compute(packet->data + 1, packet->count - 1, 0);
    crc16_value = Tranverse16(crc16_value); //字节调换
    appendData(packet, (uint8_t *)&crc16_value, 2);
}

uint32_t packetCheck(Packet_t *packet)
{
    Packet_L1_Tail_t *l1Tail;
    //没有L1Header
    if (packet->count < L1_HEADER_LENGTH)
    {
        return PACKET_NOT_FINISH;
    }
    //检验包头
    if (packet->count == L1_HEADER_LENGTH)
    {
        Packet_L1_Header_t *l1Header = (Packet_L1_Header_t *)(packet->data);
        if (l1Header->start_code != START_CODE)
        {
            return PACKET_INVALID;
        }
    }

    //数据长度不够校验
    if (packet->count < (HEADERS_LENGTH + 2))
    {
        return PACKET_NOT_FINISH;
    }
    //校验数据
    uint16_t length = packet->data[3] * 256 + packet->data[4] + 7;
    if(length > packet->buffer_size)
    {
        return PACKET_LENGTH_ERROR;
    }
    if (packet->count < length)
    {
        return PACKET_NOT_FINISH;
    }
    if (packet->count > length)
    {
        return PACKET_OVER_LENGTH;
    }
    l1Tail = (Packet_L1_Tail_t *)(packet->data + packet->count - 2);
    uint16_t crc16_value, packet_crc16;

    crc16_value = crc16_compute(packet->data + 1, packet->count - 3, 0);
    //校验成功
    packet_crc16 = l1Tail->crc16[0] * 256 + l1Tail->crc16[1];
    if (crc16_value == packet_crc16)
    {
        return PACKET_CHECK_SUCCESS;
    }
    //校验失败
    else
    {
        return PACKET_CHECK_FAULT;
    }
}
