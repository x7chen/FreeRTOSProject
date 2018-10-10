/********************************************************************
*  File Name: 
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#ifndef UHF_PROTOCOL_H
#define UHF_PROTOCOL_H
#include "uhf_packet.h"

typedef struct
{
    uint8_t epc[12];
    uint8_t pc[2];
} uhf_tag_t;

#define MAX_TAGS 100
typedef struct
{
    uhf_tag_t tags[MAX_TAGS];
    uint16_t count;
    void (*clear)(void);
    void (*add_a_tag)(uhf_tag_t *tag);
    uhf_tag_t *(*get_a_tag)(void);
} uhf_tags_t;

extern uhf_tags_t uhf_tags;
extern uint8_t uhf_read_time;
extern uint8_t uhf_read_power;
extern uint8_t uhf_read_period;
void nv_set_uhf_read_time(uint8_t *time);
void nv_set_uhf_read_power(uint8_t *power);
void nv_set_uhf_read_period(uint8_t *period);
void uhf_protocol_init(void);
void uhf_EPC_read(uint8_t power,uint32_t read_time);
void uhf_EPC_stop(void);
void uhf_cmd_start_read(void);
void uhf_cmd_stop_read(void);
void uhf_cmd_set_power(uint8_t dbm);
void uhf_data_receive(uint8_t *p_data, uint16_t length);

#endif
