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
}uhf_tag_t;

#define MAX_TAGS    100
typedef struct
{
    uhf_tag_t tags[MAX_TAGS];
    uint16_t count;
    void (*clear)(void);
    void (*add_a_tag)(uhf_tag_t *tag);
    uhf_tag_t *(*get_a_tag)(void);
}uhf_tags_t;

extern uhf_tags_t uhf_tags;

void uhf_protocol_init(void);
Packet_t *getReceivePacket(void);
Packet_t *getSendPacket(void);
void uhf_EPC_read(void);
void uhf_data_receive(uint8_t *p_data, uint16_t length);


#endif
