/********************************************************************
*  File Name: 
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#ifndef BYTEWISE_H_
#define BYTEWISE_H_
#include <stdint.h>

#define Tranverse16(X) ((((uint16_t)(X)&0xff00) >> 8) | (((uint16_t)(X)&0x00ff) << 8))
#define Tranverse32(X) ((((uint32_t)(X)&0xff000000) >> 24) | \
                        (((uint32_t)(X)&0x00ff0000) >> 8) |  \
                        (((uint32_t)(X)&0x0000ff00) << 8) |  \
                        (((uint32_t)(X)&0x000000ff) << 24))
#define Tranverse64(X) ((((uint64_t)(X)&0xff00000000000000) >> 56) | \
                        (((uint64_t)(X)&0x00ff000000000000) >> 40) | \
                        (((uint64_t)(X)&0x0000ff0000000000) << 24) | \
                        (((uint64_t)(X)&0x000000ff00000000) << 8) |  \
                        (((uint64_t)(X)&0x000000000ff00000) >> 8) |  \
                        (((uint64_t)(X)&0x0000000000ff0000) >> 24) | \
                        (((uint64_t)(X)&0x000000000000ff00) << 40) | \
                        (((uint64_t)(X)&0x00000000000000ff) << 56))

#define Combine16_LittleEndian(B1, B2)                          ((uint16_t)B2 << 8 | (uint16_t)B1)
#define Combine32_LittleEndian(B1, B2, B3, B4)                  ((uint32_t)B4 << 24 | (uint16_t)B3 << 16 | (uint32_t)B2 << 8 | (uint32_t)B1)
#define Combine64_LittleEndian(B1, B2, B3, B4, B5, B6, B7, B8)  ((uint64_t)B8 << 56 | (uint64_t)B7 << 48 | (uint64_t)B6 << 40 | (uint64_t)5 << 32 | \
                                                                (uint64_t)B4 << 24 | (uint64_t)B3 << 16 | (uint64_t)B2 << 8 | (uint64_t)B1)
uint32_t swap_bits(uint32_t inp);
uint32_t bytewise_bitswap(uint32_t inp);
#endif
