/********************************************************************
*  File Name: uhf_protocol.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include "uhf_protocol.h"
#include "uhf_packet.h"
#include "string.h"
#include "../../utils/bytewise.h"
#include "includeFreeRTOS.h"
#include "../utils/bit.h"
#include "Board_LED.h"
#include "../Drivers/log_driver/log.h"

#define STATE_IDLE 0x00
#define STATE_SENDING 0x10
#define STATE_RECEIVING 0x20

Packet_t sPacket;
Packet_t rPacket;
extern EventGroupHandle_t event_uhf;

uhf_tags_t uhf_tags;

uint8_t receivebuff[MAX_PACKET_LENGTH];
uint8_t sendbuff[MAX_PACKET_LENGTH];
uint8_t communicate_state = STATE_IDLE;

static void uhf_tags_clear(void)
{
    uhf_tags.count = 0;
}

static void uhf_add_a_tag(uhf_tag_t *tag)
{
    if (uhf_tags.count < MAX_TAGS)
    {
        for (int i = 0; i < uhf_tags.count; i++) //去重
        {
            if (memcmp(tag, uhf_tags.tags + i, sizeof(uhf_tag_t)) == 0)
            {
                return;
            }
        }
        memcpy(&uhf_tags.tags[uhf_tags.count], (uint8_t *)tag, sizeof(uhf_tag_t));
        uhf_tags.count++;
    }
}

static uhf_tag_t *uhf_get_a_tag(void)
{
    if (uhf_tags.count > 0)
    {
        uhf_tags.count--;
        return &uhf_tags.tags[uhf_tags.count];
    }
    else
    {
        return NULL;
    }
}

void uhf_protocol_init(void)
{
    packetInit(&sPacket, sendbuff);
    packetInit(&rPacket, receivebuff);
    uhf_tags.clear = uhf_tags_clear;
    uhf_tags.add_a_tag = uhf_add_a_tag;
    uhf_tags.get_a_tag = uhf_get_a_tag;
}

Packet_t *getReceivePacket(void)
{
    return &rPacket;
}

Packet_t *getSendPacket(void)
{
    return &sPacket;
}

static void uhf_send()
{
    genL1Header(&sPacket, 0);
    xEventGroupSetBits(event_uhf, BIT(2));
}

void setMid(uint8_t type, uint8_t mid)
{
    Packet_L2_Header_t l2Header;
    l2Header.type = type;
    l2Header.command = mid;
    setL2Header(&sPacket, &l2Header);
}

void append_m_value(uint8_t vl, uint8_t *data, uint16_t length)
{
    uint16_t len = Tranverse16(length);
    // vl 可变长度
    if (vl == 1)
    {
        appendData(&sPacket, (uint8_t *)&len, 2);
        appendData(&sPacket, data, length);
    }
    else
    {
        appendData(&sPacket, data, length);
    }
}
void append_p_value(uint8_t vl, uint8_t pid, uint8_t *data, uint16_t length)
{
    uint16_t len = Tranverse16(length);
    appendData(&sPacket, &pid, 1);
    if (vl == 1)
    {
        appendData(&sPacket, (uint8_t *)&len, 2);
        appendData(&sPacket, data, length);
    }
    else
    {
        appendData(&sPacket, data, length);
    }
}
/*********************************************
 * 解包，并分发数据
 ********************************************/
void uhf_resolve()
{
    uint8_t mid = rPacket.data[2];
    switch (mid)
    {
    case 0x00:
        uhf_tags.add_a_tag((uhf_tag_t *)(rPacket.data + 7));
        break;
    case 0x01:
        PIN_Low(UHF_EN);
        PIN_Low(UHF_EN2);
        LED_Off(LED_UHF);
    LOG_printf("TAG_COUNT: %d",uhf_tags.count);
        //读卡完成
        break;
    default:
        break;
    }
}
/*********************************************
 * 发送读卡指令(EPC)
 ********************************************/
void uhf_EPC_read(void)
{
    uint8_t param[] = {0x01, 0x00};

    packetFormat(&sPacket);
    setMid(0x02, 0x10);
    append_m_value(0, param, sizeof(param));
    uhf_send();
    LED_On(LED_UHF);
    uhf_tags.clear(); //读卡之前先清空数据
}

/*********************************************
 * 接收数据组包，并完成L1层验证
 ********************************************/
void uhf_data_receive(uint8_t *p_data, uint16_t length)
{
     LOG_Hex(p_data,length);
    
    static uint32_t check_result;
    uint32_t i;
    for (i = 0; i < length; i++)
    {
        appendData(&rPacket, p_data + i, 1);
        check_result = packetCheck(&rPacket);
        switch (check_result)
        {
        case PACKET_INVALID:
        case PACKET_OVER_MAXLENGTH:
        case PACKET_OVER_LENGTH:
        case PACKET_CHECK_FAULT:
            packetClear(&rPacket);
            break;
        case PACKET_CHECK_SUCCESS:
            uhf_resolve();
            packetClear(&rPacket);
            break;
        default:
            break;
        }
    }
    
}
