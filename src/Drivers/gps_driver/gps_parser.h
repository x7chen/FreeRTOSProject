/********************************************************************
*  File Name: 
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#ifndef GPS_PARSER_H
#define GPS_PARSER_H
#include <stdint.h>
#include "../../utils/buffer.h"

typedef struct
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} DATE_TIME;

typedef struct
{
    char status;
    double latitude;                //纬度
    double longitude;               //经度
    int latitude_Degree;            //度
    int latitude_Cent;              //分
    int latitude_Second;            //秒
    int longitude_Degree;           //度
    int longitude_Cent;             //分
    int longitude_Second;           //秒
    float speed;                    //速度
    float direction;                //航向
    float height;                   //海拔高度
    int satellite;
    uint8_t NS;
    uint8_t EW;
    DATE_TIME D;
} GPS_INFO;

#define GPS_LINE_SIZE     256

extern uint8_t gps_post_period;
extern uint8_t gps_power;
extern GPS_INFO GPS;
extern buffer_t gps_buffer;
int GPS_RMC_Parse(const char *line, GPS_INFO *GPS);
void GPS_init(void);
void GPS_Receive(uint8_t *data,uint16_t length);
void GPS_Test(void);

#endif
