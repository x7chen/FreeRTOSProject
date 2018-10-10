/********************************************************************
*  File Name: nb_coap.c
*  Purpose: NB-IOT数据打包
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "../../utils/asciicvt.h"
#include "Board_LED.h"
#include "nb_coap.h"
#include "Driver_USART.h"
#include "../gps_driver/gps_parser.h"
#include "../../utils/clock/clock.h"
#include "../uhf_driver/uhf_protocol.h"
#include "includeFreeRTOS.h"
#include "../../utils/bytewise.h"
#include "../../utils/bit.h"
#include "../../utils/delay.h"
#include "../../utils/checksum.h"
#include "../../utils/buffer.h"
#include "../Drivers/log_driver/log.h"
#include "../nv_data/nv_data.h"
#include "../main/compiler_abstraction.h"

#define nb_delay(ms) vTaskDelay(ms)
extern uint8_t nb_cmd_read_tag;
buffer_t nb_rx_buffer = BUFFER_DEFAULT;
buffer_t coap_buffer = BUFFER_DEFAULT;
char enter[] = "\r\n";
uint8_t nb_tx_data[SEND_BUFFER_SIZE];
uint8_t nb_rx_data[RECEIVE_BUFFER_SIZE];

uint8_t nb_tx_orgin_data[512];
uint8_t nb_rx_orgin_data[128];

extern EventGroupHandle_t event_log;
extern EventGroupHandle_t event_nb;
extern EventGroupHandle_t event_uhf;
extern EventGroupHandle_t event_gps;
extern EventGroupHandle_t event_bt;

uint32_t box_id;
uint64_t imei;
uint8_t lock_status;
uint8_t battery = 87;
UTCTimeStruct *utc;
uint8_t lock_open = 0;
uint8_t coap_is_busy = 0;
extern ARM_DRIVER_USART *USARTdrv2;
volatile nb_cmd_t current_cmd = DEFAULT;
int CoAP_Post(char *data, uint16_t length);
void ReceiveDataParser(void);

void nv_setBoxNum(uint8_t *num)
{
    memcpy((uint8_t *)&box_id, num, 4);
}
void nv_getBoxNum(uint8_t *num)
{
    memcpy(num, (uint8_t *)&box_id, 4);
}

void NB_CMD(char *cmd)
{
    
    if (USARTdrv2->GetStatus().tx_busy == 0)
    {
        USARTdrv2->Send(cmd, strlen(cmd));
    }
    // LOG_Ascii(cmd,strlen(cmd));
    nb_rx_buffer.clear(&nb_rx_buffer);
}
void Init_NB05(void *args)
{
    nb_rx_buffer.initialize(&nb_rx_buffer, (uint8_t *)nb_rx_data, sizeof(nb_rx_data));
    coap_buffer.initialize(&coap_buffer, (uint8_t *)nb_tx_orgin_data, sizeof(nb_tx_orgin_data));

    PIN_High(NB_EN);
    nb_delay(3000);
    NB_CMD("AT+CFUN=0\r\n"); //关闭射频
    current_cmd = CFUN;
    nb_delay(1000);
    NB_CMD("AT+CGMR\r\n"); //查询版本号
    current_cmd = CGMR;
    nb_delay(1000);
    NB_CMD("AT+NBAND?\r\n"); //查询波段
    current_cmd = NBAND;
    nb_delay(1000);
    NB_CMD("AT+NBAND=5\r\n"); //设置波段
    current_cmd = NBAND;
    nb_delay(1000);
    NB_CMD("AT+CGSN=1\r\n"); //查询IMEI
    current_cmd = CGSN;
    nb_delay(1000);
    NB_CMD("AT+NCDP=180.101.147.115,5683\r\n"); //设置平台IP
    current_cmd = NCDP;
    nb_delay(1000);
    NB_CMD("AT+NRB\r\n"); //软重启
    current_cmd = NRB;
    nb_delay(10000);
    NB_CMD("AT+CFUN=1\r\n"); //打开射频
    current_cmd = CFUN;
    nb_delay(1000);
    NB_CMD("AT+CIMI\r\n"); //查询SIM卡信息
    current_cmd = CIMI;
    nb_delay(1000);
    NB_CMD("AT+CMEE=1\r\n"); //打开错误报告
    current_cmd = CMEE;
    nb_delay(1000);
    NB_CMD("AT+CGDCONT=1,\"IP\",\"ctnet\"\r\n"); //设置APN
    current_cmd = CGDCONT;
    nb_delay(1000);
    NB_CMD("AT+CSCON=1\r\n"); //设置基站连接通知
    current_cmd = CSCON;
    nb_delay(1000);
    NB_CMD("AT+CEREG=2\r\n"); //设置连接核心网通知
    current_cmd = CEREG;
    nb_delay(1000);
    NB_CMD("AT+NNMI=1\r\n"); //开启下行数据通知
    current_cmd = NNMI;
    nb_delay(1000);
    NB_CMD("AT+CGATT=1\r\n"); //自动搜网
    current_cmd = CGATT;
    nb_delay(1000);
    NB_CMD("AT+NUESTATS\r\n"); //查询UE状态
    current_cmd = NUESTATS;
    nb_delay(1000);
    NB_CMD("AT+CGPADDR\r\n"); //查询分配的IP
    current_cmd = CGPADDR;
    nb_delay(1000);
    LED_On(LED_NB);
    vTaskDelete(xTaskGetCurrentTaskHandle());
}

void lock_control(void)
{
    if (lock_open)
    {
        lock_open = 0;
        PIN_High(BP_EN);
        LED_On(LED_1);
        vTaskDelay(200);
        PIN_High(LOCK);
        vTaskDelay(8);
        PIN_Low(LOCK);
        PIN_Low(BP_EN);
        LED_Off(LED_1);
    }
}
void Soft_Reset()
{
    __disable_irq();
    NVIC_SystemReset();//请求单片机重启
}

const uint8_t default_ack[4] = {0xAA, 0xAA, 0x00, 0x00};
const uint8_t param_reset[4] = {0x00, 0x00, 0x00, 0x00};
void NB_CMD_Process(uint8_t *data, uint16_t length)
{
    if (length == 4 && memcmp(default_ack, data, length) == 0)
    {
        return;
    }
    if (length >= 1)
    {
        switch (data[0])
        {
        case 0x00:
            xEventGroupSetBits(event_uhf, BIT(1)); //uhf读卡
            break;
        case 0x01:
            lock_open = 1;
            break;
        case 0x02:
            gps_power = 1;
            save_config();
            break;
        case 0x03:
            gps_power = 0;
            save_config();
            break;
        case 0x04:
            if (length >= 5)
            {
                if(memcmp(data+1,param_reset,4)==0)
                {
                    uhf_read_period = 0;        // 默认读卡间隔(单位:10秒):         0 秒
                    gps_post_period = 10;       // 默认GPS上报间隔(单位:1秒):       10 秒
                    uhf_read_power = 10;        // 默认读卡功率(单位:1dBm):         10 dBm
                    uhf_read_time = 10;         // 默认读卡时间(单位:100毫秒):      1000 毫秒
                }
                if (data[1] <= 255)
                {
                    uhf_read_period = data[1];
                }
                
                if(data[2] >= 10 && data[2] <= 200)
                {
                    gps_post_period = data[2];
                }
                
                if (data[3] > 0 && data[3] <= 36)
                {
                    uhf_read_power = data[3];
                }
                
                if (data[4] >= 3 && data[4] <= 200)
                {
                    uhf_read_time = data[4];
                }
                box_parameter_post();
                save_config();
            }
            break;
        case 0x05:
            box_parameter_post();
            break;
        case 0x06:
            Soft_Reset();
            break;
        default:
            break;
        }
    }
}

void ReceiveDataParser(void)
{
    char *s;
    int length;
#if 0
    LOG_Ascii((char *)(nb_rx_buffer.data),nb_rx_buffer.count);
#else
    if (nb_rx_buffer.count < 8)
    {
        nb_rx_buffer.clear(&nb_rx_buffer);
        return;
    }
    switch ((uint8_t)current_cmd)
    {
    case NMGS:
        /***************************
             *  解析NB接收的数据
             *  +NNMI:3,A0B0C0
             *  p     l d 
             *
             ***************************/
        s = strstr((char *)(nb_rx_buffer.data), "+NNMI:");
        if (s == NULL)
        {
            break;
        }
        s = strchr(s, ':') + 1;
        length = strtol(s, NULL, 10);
        if (length <= 0 || length > 32)
        {
            break;
        }
        s = strchr(s, ',') + 1;

        Str2Hex(s, (char *)nb_rx_orgin_data, length * 2);
        NB_CMD_Process((uint8_t *)nb_rx_orgin_data, length);
        LED_Off(LED_NB);
        //  这里数据已经接收完成
        LOG_Ascii("NB_Receive:", 11);
        LOG_Hex((uint8_t *)nb_rx_orgin_data, length);
        break;
    case CCLK:
        s = strstr((char *)(nb_rx_buffer.data), "+CCLK:");
        if (s == NULL)
        {
            break;
        }
        utc = get_clock_time();
        s = strchr(s, ':') + 1;
        utc->year = strtol(s, NULL, 10);
        s = strchr(s, '/') + 1;
        utc->month = strtol(s, NULL, 10);
        s = strchr(s, '/') + 1;
        utc->day = strtol(s, NULL, 10);
        s = strchr(s, ',') + 1;
        utc->hour = strtol(s, NULL, 10);
        s = strchr(s, ':') + 1;
        utc->minutes = strtol(s, NULL, 10);
        s = strchr(s, ':') + 1;
        utc->seconds = strtol(s, NULL, 10);
        set_clock_time_counter(convert_time_to_Second(utc));
        set_time_zone(8);
        break;
    case CGSN:
        s = strstr((char *)(nb_rx_buffer.data), "+CGSN:");
        if (s == NULL)
        {
            break;
        }
        s = strchr(s, ':') + 1;
        imei = strtoull(s, NULL, 10);
        break;
    default:
        break;
    }
        // current_cmd = NMGS;
#endif
    nb_rx_buffer.clear(&nb_rx_buffer);
}

int CoAP_Post(char *data, uint16_t length)
{
    memset(nb_tx_data, 0, sizeof(nb_tx_data));
    sprintf((char *)nb_tx_data, "AT+NMGS=%d,", length);
    Hex2Str(data, (char *)nb_tx_data + strlen((char *)nb_tx_data), length);
    memcpy(nb_tx_data + strlen((char *)nb_tx_data), enter, 2);
    NB_CMD((char *)nb_tx_data);
    current_cmd = NMGS;
    LED_On(LED_NB);
    return 0;
}

void box_info_post(void)
{
    
    //    uint8_t check_sum = 0;
    coap_buffer.clear(&coap_buffer);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\xAA\x00", 2);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&box_id, 4);

    coap_buffer.append_data(&coap_buffer, (uint8_t *)&lock_status, 1);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&battery, 1);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&(uhf_tags.count), 2);
    for (int i = 0; i < uhf_tags.count; i++)
    {
        coap_buffer.append_data(&coap_buffer, (uint8_t *)&(uhf_tags.tags[i]), 12);
    }
    //    check_sum = checksum(coap_buffer.data, coap_buffer.count);
    //    coap_buffer.append_data(&coap_buffer, &check_sum, 1);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\x0A", 1);
    LOG_Ascii("NB_Send:", 8);
    LOG_Hex(coap_buffer.data, coap_buffer.count);
    CoAP_Post((char *)coap_buffer.data, coap_buffer.count);
}

void box_location_post(void)
{
    //    uint8_t check_sum = 0;
    // GPS_Test();

    coap_buffer.clear(&coap_buffer);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\xAA\x02", 2);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&box_id, 4);
//    coap_buffer.append_data(&coap_buffer, (uint8_t *)&(GPS.longitude), 8);
//    coap_buffer.append_data(&coap_buffer, &(GPS.EW), 1);
//    coap_buffer.append_data(&coap_buffer, (uint8_t *)&(GPS.latitude), 8);
//    coap_buffer.append_data(&coap_buffer, &(GPS.NS), 1);
    coap_buffer.append_data(&coap_buffer, gps_buffer.data,gps_buffer.count);
    gps_buffer.clear(&gps_buffer);
    //    check_sum = checksum(coap_buffer.data, coap_buffer.count);
    //    coap_buffer.append_data(&coap_buffer, &check_sum, 1);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\x0A", 1);
    LOG_Ascii("NB_Send:", 8);
    LOG_Hex(coap_buffer.data, coap_buffer.count);
    LOG_printf("longitude:%f%c,latitude:%f%c", GPS.longitude, GPS.EW, GPS.latitude, GPS.NS);
    CoAP_Post((char *)coap_buffer.data, coap_buffer.count);
}

void box_parameter_post()
{
    coap_buffer.clear(&coap_buffer);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\xAA\x04", 2);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&box_id, 4);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&uhf_read_period, 1);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&gps_post_period, 1);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&uhf_read_power, 1);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&uhf_read_time, 1);
    
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\x0A", 1);
    CoAP_Post((char *)coap_buffer.data, coap_buffer.count);
}

void heart_beat_post()
{
    coap_buffer.clear(&coap_buffer);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\xAA\x01", 2);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)&box_id, 4);
    coap_buffer.append_data(&coap_buffer, (uint8_t *)"\x0A", 1);
    CoAP_Post((char *)coap_buffer.data, coap_buffer.count);
}

void calibrate_time()
{
    NB_CMD("AT+CCLK?\r\n"); //获取时间
    current_cmd = CCLK;
}
