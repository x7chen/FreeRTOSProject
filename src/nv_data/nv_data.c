/********************************************************************
*  File Name: nv_data.c
*  Purpose: 非易失数据保存和载入
*  Author: Sean
*  Date: 2018-06-20
*********************************************************************/

#include <stdint.h>
#include <string.h>
#include "../main/compiler_abstraction.h"
#include "stm32f10x.h"
#include "nv_data.h"
#define CONFIG_TABLE_SIZE       2048
#define WORD_OF_CONFIG_TABLE    (CONFIG_TABLE_SIZE/4)
__ALIGN(4) uint8_t config_table[CONFIG_TABLE_SIZE];


__WEAK void nv_setSN(uint8_t * sn){}
__WEAK void nv_getSN(uint8_t * sn){}
__WEAK void nv_setBoxNum(uint8_t * num){}
__WEAK void nv_getBoxNum(uint8_t * num){}
__WEAK void nv_set_gps_post_period(uint8_t *gps_post_period){}
__WEAK void nv_get_gps_post_period(uint8_t *gps_post_period){}
__WEAK void nv_set_gps_power(uint8_t *gps_power){}
__WEAK void nv_get_gps_power(uint8_t *gps_power){}
__WEAK void nv_set_uhf_read_time(uint8_t *time){}
__WEAK void nv_get_uhf_read_time(uint8_t *time){}
__WEAK void nv_set_uhf_read_power(uint8_t *power){}
__WEAK void nv_get_uhf_read_power(uint8_t *power){}
__WEAK void nv_set_uhf_read_period(uint8_t *period){}
__WEAK void nv_get_uhf_read_period(uint8_t *period){}    
/******************************************************
 *  save_config_table
 *  写flash
 *****************************************************/
static void save_config_table()
{
    uint32_t NbrOfPage = 0x00;
    uint32_t EraseCounter = 0x00;
    uint32_t BaseAddress = 0x00;
    volatile FLASH_Status FLASHStatus = FLASH_COMPLETE;
    /* Porgram FLASH Bank1 ********************************************************/
    /* Unlock the Flash Bank1 Program Erase controller */
    FLASH_UnlockBank1();

    /* Define the number of page to be erased */
    NbrOfPage = (BANK1_WRITE_END_ADDR - BANK1_WRITE_START_ADDR) / FLASH_PAGE_SIZE;

    /* Clear All pending flags */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    /* Erase the FLASH pages */
    for(EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
    {
        FLASHStatus = FLASH_ErasePage(BANK1_WRITE_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter));
    }

    /* Program Flash Bank1 */
    BaseAddress = BANK1_WRITE_START_ADDR;

//    while((BaseAddress < BANK1_WRITE_END_ADDR) && (FLASHStatus == FLASH_COMPLETE))
//    {
//        FLASHStatus = FLASH_ProgramWord(BaseAddress, *(uint32_t *)config_table+4);
//        BaseAddress = BaseAddress + 4;
//    }
    uint32_t index = 0;
    while((BaseAddress < BANK1_WRITE_END_ADDR) && (FLASHStatus == FLASH_COMPLETE))
    {
        FLASHStatus = FLASH_ProgramWord(BaseAddress, *((uint32_t *)config_table+index));
        BaseAddress = BaseAddress + 4;
        index += 1;
    }
    FLASH_LockBank1();
}

/******************************************************
 *  save_config
 *  保存参数，用于记录当前设置状态
 *****************************************************/
void save_config()
{
    /*************************************************
     * 收集当前运行参数列表
     *************************************************/
    nv_getSN(config_table+CONFIG_SN_OFFSET);
    nv_getBoxNum(config_table+CONFIG_BOX_NUM_OFFSET);
    nv_get_gps_post_period(config_table+CONFIG_GPS_POST_PERIOD_OFFSET);
    nv_get_gps_power(config_table+CONFIG_GPS_POWER_OFFSET);
    nv_get_uhf_read_power(config_table+CONFIG_UHF_READ_POWER_OFFSET);
    nv_get_uhf_read_time(config_table+CONFIG_UHF_READ_TIME_OFFSET);
    nv_set_uhf_read_period(config_table+CONFIG_UHF_READ_PERIOD_OFFSET);
    
    save_config_table();
    
}

/******************************************************
 *  load_config
 *  载入配置参数，用于开机初始化
 *****************************************************/
void load_config()
{
    uint32_t BaseAddress = 0x00;
    /* Check the correctness of written data */
    BaseAddress = BANK1_WRITE_START_ADDR;
    
    memcpy(config_table,(uint8_t *)BaseAddress,CONFIG_TABLE_SIZE);
    /*************************************************
     * 把参数列表初始化到系统中去
     *************************************************/
    nv_setSN((uint8_t *)BaseAddress+CONFIG_SN_OFFSET);
    nv_setBoxNum((uint8_t *)BaseAddress+CONFIG_BOX_NUM_OFFSET);
    nv_set_gps_post_period((uint8_t *)BaseAddress+CONFIG_GPS_POST_PERIOD_OFFSET);
    nv_set_gps_power((uint8_t *)BaseAddress+CONFIG_GPS_POWER_OFFSET);
    nv_set_uhf_read_power((uint8_t *)BaseAddress+CONFIG_UHF_READ_POWER_OFFSET);
    nv_set_uhf_read_time((uint8_t *)BaseAddress+CONFIG_UHF_READ_TIME_OFFSET);
    nv_set_uhf_read_period((uint8_t *)BaseAddress+CONFIG_UHF_READ_PERIOD_OFFSET);
}

/******************************************************
 *  set_config
 *  用于批量设置数据，通常用于生产或者上位机程序
 *****************************************************/
void set_config(uint32_t offset,uint8_t *data,uint32_t length)
{
    uint32_t BaseAddress = 0x00;
    /* Check the correctness of written data */
    if(offset+length>=CONFIG_TABLE_SIZE)
    {
        return;
    }
    BaseAddress = BANK1_WRITE_START_ADDR;
    memcpy(config_table,(uint8_t *)BaseAddress,CONFIG_TABLE_SIZE);
    memcpy(config_table+offset,data,length);

    save_config_table();
}
/******************************************************
 *  get_config
 *  用于读取所有参数，通常用于生产或者上位机程序
 *****************************************************/
uint8_t * get_config()
{
    return (uint8_t *)BANK1_WRITE_START_ADDR;
}



