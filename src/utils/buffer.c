#include <stdint.h>
#include <string.h>
#include "buffer.h"

void buffer_clear(buffer_t *buffer)
{
    buffer->count = 0;
    memset(buffer->data,0,buffer->buffer_size);
}

void buffer_append_data(buffer_t *buffer,uint8_t *data, uint16_t length)
{
    uint16_t len;
    
    if (buffer->count + length >= buffer->buffer_size)
    {
        len = buffer->buffer_size - buffer->count;
    }
    else
    {
        len = length;
    }
    memcpy(buffer->data + buffer->count, data, len);
    buffer->count += len;
}

uint32_t buffer_set_data(buffer_t *buffer, uint16_t offset, uint8_t *data, uint16_t length)
{
    if (offset + length >= buffer->buffer_size)
    {
        return BUFFER_OVER_MAXSIZE;
    }
    memcpy(buffer->data + offset, data, length);
    if(buffer->count < (offset + length))
    {
        buffer->count = (offset + length);
    }
    return BUFFER_SUCCESS;
}

void buffer_initialize(buffer_t *buffer,uint8_t *data,uint16_t length)
{
    buffer->data = data;
    buffer->buffer_size =length;    
    buffer->count = 0;
    buffer->initialize = (buffer_initialize_t)buffer_initialize;
    buffer->append_data = (buffer_append_data_t)buffer_append_data;
    buffer->set_data = (buffer_set_data_t)buffer_set_data;
    buffer->clear = (buffer_clear_t)buffer_clear;
    buffer->clear(buffer);
}


