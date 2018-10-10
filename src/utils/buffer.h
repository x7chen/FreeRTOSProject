#ifndef  BUFFER_H_
#define  BUFFER_H_
#include <stdint.h>

typedef void (*buffer_clear_t)(void *);
typedef void (*buffer_append_data_t)(void * ,uint8_t * , uint16_t);
typedef void (*buffer_initialize_t)(void *buffer,uint8_t *data,uint16_t length);
typedef uint32_t (*buffer_set_data_t)(void *buffer, uint16_t offset, uint8_t *data, uint16_t length);
typedef struct
{
    uint8_t *data;
    uint16_t count;
    uint16_t buffer_size;
    buffer_initialize_t initialize;
    buffer_clear_t clear;
    buffer_append_data_t append_data; 
    buffer_set_data_t set_data; 
} buffer_t;

void buffer_clear(buffer_t *buffer);
void buffer_append_data(buffer_t *buffer,uint8_t *data, uint16_t length);
void buffer_initialize(buffer_t *buffer,uint8_t *data,uint16_t length);
uint32_t buffer_set_data(buffer_t *buffer, uint16_t offset, uint8_t *data, uint16_t length);

#define BUFFER_DEFAULT  {NULL, 0, 0, \
                        (buffer_initialize_t)buffer_initialize, \
                        (buffer_clear_t)buffer_clear, \
                        (buffer_append_data_t)buffer_append_data,\
                        (buffer_set_data_t)buffer_set_data}

#define BUFFER_DEFAULT0  {NULL, 0, 0, (buffer_initialize_t)buffer_initialize, NULL, NULL}

#define BUFFER_ERROR_BASE       0x00030000

#define BUFFER_SUCCESS          (BUFFER_ERROR_BASE + 0)
#define BUFFER_OVER_MAXSIZE     (BUFFER_ERROR_BASE + 1)


#endif


