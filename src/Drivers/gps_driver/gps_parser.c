/********************************************************************
*  File Name: gps_parser.c
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gps_parser.h"
#include "Board_LED.h"

GPS_INFO GPS;
buffer_t gps_line = BUFFER_DEFAULT;
uint8_t gps_line_data[GPS_LINE_SIZE];

uint8_t gps_post_period = 10;
uint8_t gps_power = 0;

buffer_t gps_buffer  = BUFFER_DEFAULT;
uint8_t gps_buffer_data[360];

void nv_set_gps_post_period(uint8_t *period)
{
    gps_post_period = *period;
    if(gps_post_period < 10 || gps_post_period > 200)
    {
        gps_post_period = 10;
    }
}

void nv_get_gps_post_period(uint8_t *period)
{
    *period = gps_post_period;
}

void nv_set_gps_power(uint8_t *power)
{
    gps_power = *power;
    if(gps_power > 1)
    {
        gps_power = 0;
    }
}

void nv_get_gps_power(uint8_t *power)
{
    *power = gps_power;
}

void GPS_init(void)
{
    gps_line.initialize(&gps_line,gps_line_data,sizeof(gps_line_data));
    gps_buffer.initialize(&gps_buffer,gps_buffer_data,sizeof(gps_buffer_data));
}
uint64_t gps_count = 0;
void GPS_Receive(uint8_t *data, uint16_t length)
{
    
    for (int i = 0; i < length; i++)
    {
        if (data[i] == '\n')
        {
            if (strstr((char *)(gps_line.data), "GNRMC") != 0)
            {
                //LED_On(LED_1);
                gps_count ++;
                if (GPS_RMC_Parse((char *)(gps_line.data), &GPS) == 1)
                {
                    LED_On(LED_GPS);
                    GPS.status = 1;
                    if(gps_count%2==0)
                    {
                        gps_buffer.append_data(&gps_buffer, (uint8_t *)&(GPS.longitude), 8);
                        gps_buffer.append_data(&gps_buffer, &(GPS.EW), 1);
                        gps_buffer.append_data(&gps_buffer, (uint8_t *)&(GPS.latitude), 8);
                        gps_buffer.append_data(&gps_buffer, &(GPS.NS), 1);
                    }
                }
                else
                {
                    LED_Off(LED_GPS);
                    GPS.status = 0;
                }
            }
            else
            {
                // LED_Off(LED_1);
            }

            gps_line.clear(&gps_line);
        }
        else
        {
            gps_line.append_data(&gps_line,data + i, 1);
        }
    }
}
double Get_Double_Number(char *str)
{
    return strtod(str, NULL);
}
double Get_Float_Number(char *str)
{
    return strtof(str, NULL);
}

int Get_Int_Number(char *str)
{
    return strtol(str, NULL, 10);
}

char *GetComma(uint8_t index, char *str)
{
    char *point = str;
    for (int i = 0; i < index; i++)
    {
        point = strchr(point, ',');
        point++;
    }
    return point;
}

/* *****************************************************************
 * 数据用"\r\n"来分割
 *******************************************************************/
int GPS_RMC_Parse(const char *line, GPS_INFO *GPS)
{
    uint8_t status;
    char *time;
    float lati_cent_tmp, lati_second_tmp;
    float long_cent_tmp, long_second_tmp;
    float speed_tmp;
    char *buf = (char *)line;
    status = *GetComma(2, buf);

    if (status == 'A') //如果数据有效，则分析
    {
        GPS->NS = *GetComma(4, buf);
        GPS->EW = *GetComma(6, buf);

        GPS->latitude = Get_Double_Number(GetComma(3, buf));
        GPS->longitude = Get_Double_Number(GetComma(5, buf));

        GPS->latitude_Degree = (int)GPS->latitude / 100; //分离纬度
        lati_cent_tmp = (GPS->latitude - GPS->latitude_Degree * 100);
        GPS->latitude_Cent = (int)lati_cent_tmp;
        lati_second_tmp = (lati_cent_tmp - GPS->latitude_Cent) * 60;
        GPS->latitude_Second = (int)lati_second_tmp;

        GPS->longitude_Degree = (int)GPS->longitude / 100; //分离经度
        long_cent_tmp = (GPS->longitude - GPS->longitude_Degree * 100);
        GPS->longitude_Cent = (int)long_cent_tmp;
        long_second_tmp = (long_cent_tmp - GPS->longitude_Cent) * 60;
        GPS->longitude_Second = (int)long_second_tmp;

        // 转换度分格式为度格式
        GPS->latitude = (int)GPS->latitude / 100 + fmod(GPS->latitude, 100.0) / 60;
        GPS->longitude = (int)GPS->longitude / 100 + fmod(GPS->longitude, 100.0) / 60;

        speed_tmp = Get_Float_Number(GetComma(7, buf));      //速度(单位：海里/时)
        GPS->speed = speed_tmp * 1.85;                       //1海里=1.85公里
        GPS->direction = Get_Float_Number(GetComma(8, buf)); //角度

        GPS->D.hour = (buf[7] - '0') * 10 + (buf[8] - '0'); //时间
        GPS->D.minute = (buf[9] - '0') * 10 + (buf[10] - '0');
        GPS->D.second = (buf[11] - '0') * 10 + (buf[12] - '0');
        time = GetComma(9, buf);
        GPS->D.day = (time[0] - '0') * 10 + (time[1] - '0'); //日期
        GPS->D.month = (time[2] - '0') * 10 + (time[3] - '0');
        GPS->D.year = (time[4] - '0') * 10 + (time[5] - '0') + 2000;

        return 1;
    }

    return 0;
}
const char teststr[] = "$GPRMC,083559.00,A,2232.11437,N,11403.91522,E,0.004,77.52,091202,,,A,V*57";

void GPS_Test()
{
    GPS_RMC_Parse(teststr, &GPS);
}
