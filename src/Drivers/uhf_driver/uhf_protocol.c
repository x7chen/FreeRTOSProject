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
#include "../Drivers/nb_driver/nb_coap.h"
#include "Driver_USART.h"

extern ARM_DRIVER_USART *USARTdrv3;

#define STATE_IDLE 0x00
#define STATE_SENDING 0x10
#define STATE_RECEIVING 0x20

Packet_t sPacket = BUFFER_DEFAULT;
Packet_t rPacket = BUFFER_DEFAULT;

uhf_tags_t uhf_tags;

uint8_t uhf_read_time = 10; //单位100ms
uint8_t uhf_read_power = 0;
uint8_t uhf_read_period = 0;

uint8_t receivebuff[MAX_PACKET_LENGTH];
uint8_t sendbuff[MAX_PACKET_LENGTH];
uint8_t communicate_state = STATE_IDLE;

void Timer3Start(uint32_t timerDelay);

void nv_set_uhf_read_time(uint8_t *time)
{
    
    if (uhf_read_time < 3 || uhf_read_time > 200)
    {
        uhf_read_time = 10;   // 单位100ms
    }
}

void nv_get_uhf_read_time(uint8_t *time)
{
    *time = uhf_read_time;
}

void nv_set_uhf_read_power(uint8_t *power)
{
    uhf_read_power = *power;
    if (uhf_read_power > 36)
    {
        uhf_read_power = 10;    // 单位dBm
    }
}

void nv_get_uhf_read_power(uint8_t *power)
{
    *power = uhf_read_power;
}

void nv_set_uhf_read_period(uint8_t *period)
{
    uhf_read_period = *period;
    if (uhf_read_period > 200)
    {
        uhf_read_period = 0;           //单位 10秒
    }
}
void nv_get_uhf_read_period(uint8_t *period)
{
    *period = uhf_read_period;
}
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
    packetInit(&sPacket, sendbuff, sizeof(sendbuff));
    packetInit(&rPacket, receivebuff, sizeof(sendbuff));
    uhf_tags.clear = uhf_tags_clear;
    uhf_tags.add_a_tag = uhf_add_a_tag;
    uhf_tags.get_a_tag = uhf_get_a_tag;
}

static void uhf_send()
{
    genL1Header(&sPacket, 0);
    USARTdrv3->Send(sPacket.data, sPacket.count);
    // LOG_Hex(sPacket.data, sPacket.length);
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

        //指令执行成功
        break;
    default:
        break;
    }
}
/*********************************************
 * 开始读卡(EPC)   开启电源，设置功率，读卡时间
 ********************************************/
void uhf_EPC_read(uint8_t power, uint32_t read_time)
{
    PIN_High(UHF_EN);
    PIN_High(UHF_EN2);
    vTaskDelay(500);
    uhf_cmd_set_power(power);
    vTaskDelay(100);
    uhf_cmd_start_read();
    LED_On(LED_UHF);
    uhf_tags.clear(); //读卡之前先清空数据
    Timer3Start(read_time);
}
/*********************************************
 * 停止读卡(EPC) 关闭电源
 ********************************************/
void uhf_EPC_stop(void)
{
    packetFormat(&sPacket);
    setMid(0x02, 0xFF);
    uhf_send();
    vTaskDelay(500);
    PIN_Low(UHF_EN);
    PIN_Low(UHF_EN2);
    LED_Off(LED_UHF);
    LOG_printf("TAG_COUNT: %d", uhf_tags.count);
    box_info_post();
}

void uhf_cmd_start_read(void)
{
    uint8_t param[] = {0x01, 0x01};
    packetFormat(&sPacket);
    setMid(0x02, 0x10);
    append_m_value(0, param, sizeof(param));
    uhf_send();
}
void uhf_cmd_stop_read(void)
{
    packetFormat(&sPacket);
    setMid(0x02, 0xFF);
    uhf_send();
}

void uhf_cmd_set_power(uint8_t dbm)
{
    uint8_t param[] = {dbm};
    packetFormat(&sPacket);
    setMid(0x02, 0x01);
    append_p_value(0, 0x01, param, sizeof(param));
    uhf_send();
}
/*********************************************
 * 接收数据组包，并完成L1层验证
 ********************************************/
void uhf_data_receive(uint8_t *p_data, uint16_t length)
{
    //LOG_Hex(p_data,length);

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
        case PACKET_LENGTH_ERROR:
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
