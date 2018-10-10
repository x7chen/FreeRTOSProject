/********************************************************************
*  File Name: 
*  Purpose:
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#ifndef _NV_DATA_H_
#define _NV_DATA_H_
#include <stdint.h>

#if defined (STM32F10X_HD) || defined (STM32F10X_HD_VL) || defined (STM32F10X_CL) || defined (STM32F10X_XL)
  #define FLASH_PAGE_SIZE    ((uint16_t)0x800)
#else
  #define FLASH_PAGE_SIZE    ((uint16_t)0x400)
#endif

#define BANK1_WRITE_START_ADDR  ((uint32_t)0x0801F800)		//126K-128K
#define BANK1_WRITE_END_ADDR    ((uint32_t)0x08020000)

#define CONFIG_SN_OFFSET				    0x0000
#define CONFIG_SN_SIZE				        12
#define CONFIG_BOX_NUM_OFFSET				0x0010
#define CONFIG_BOX_NUM_SIZE				    4
#define CONFIG_GPS_POST_PERIOD_OFFSET		0x0020
#define CONFIG_GPS_POST_PERIOD_SIZE		    1
#define CONFIG_GPS_POWER_OFFSET				0x0021
#define CONFIG_GPS_POWER_SIZE			    1
#define CONFIG_UHF_READ_POWER_OFFSET		0x0030
#define CONFIG_UHF_READ_POWER_SIZE		    1
#define CONFIG_UHF_READ_PERIOD_OFFSET		0x0031
#define CONFIG_UHF_READ_PERIOD_SIZE         1
#define CONFIG_UHF_READ_TIME_OFFSET		    0x0032
#define CONFIG_UHF_READ_TIME_SIZE			1

void save_config(void);
void load_config(void);
void set_config(uint32_t offset,uint8_t *data,uint32_t length);
uint8_t * get_config(void);

#endif

