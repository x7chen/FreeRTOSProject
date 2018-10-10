#ifndef CRC16_H__
#define CRC16_H__

#include <stdint.h>

/**@brief Function for calculating CRC-16 in blocks.
 *
 * Feed each consecutive data block into this function, along with the current value of p_crc as 
 * returned by the previous call of this function. The first call of this function should pass NULL 
 * as the initial value of the crc in p_crc.
 *
 * @param[in] p_data The input data block for computation.
 * @param[in] size   The size of the input data block in bytes.
 * @param[in] p_crc  The previous calculated CRC-16 value or NULL if first call.  
 *
 * @return The updated CRC-16 value, based on the input supplied.
 */
uint16_t crc16_compute(const uint8_t * p_data, uint32_t size, const uint16_t * original);

#endif // CRC16_H__
 
/** @} */
