#include <stdint.h>

uint16_t average16(uint16_t *data,uint16_t size)
{
    uint64_t sum = 0;
    for(int i=0;i<size;i++)
    {
        sum+=data[i];
    }
    return (uint16_t)(sum/size);
}

