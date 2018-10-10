/********************************************************************
*  File Name: 
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#ifndef NB_COAP_H
#define NB_COAP_H
#include <stdint.h>
#include "../../utils/buffer.h"
#define SEND_BUFFER_SIZE 1024
#define RECEIVE_BUFFER_SIZE 100

typedef enum
{
    DEFAULT,
    CFUN,
    CGMR,
    NBAND,
    CGSN,
    NCDP,
    NRB,
    CIMI,
    CMEE,
    CGDCONT,
    CSCON,
    CEREG,
    NNMI,
    CGATT,
    NUESTATS,
    CGPADDR,
    NMGS,
    CCLK
} nb_cmd_t;

extern buffer_t nb_rx_buffer;
extern int nb_have_data;
extern int nb_need_post;
extern int nb_need_show;
extern char share_data[50];
int CoAP_Post(char *data, uint16_t length);
void ReceiveDataParser(void);
void Init_NB05(void *args);
void NB_Receive(uint8_t *data, int length);
void box_info_post(void);
void box_location_post(void);
void box_parameter_post(void);
void heart_beat_post(void);
void calibrate_time(void);
void lock_control(void);
void nv_setBoxNum(uint8_t *num);
void nv_getBoxNum(uint8_t *num);
#endif
