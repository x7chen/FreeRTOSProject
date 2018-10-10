/********************************************************************
*  File Name: ADC_MCBSTM32E.c
*  Purpose: 电池电压检测，计算电量百分比
*  Author: Sean
*  Date: 2018-07-14
*********************************************************************/
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include "DMA_STM32F10x.h"
#include "GPIO_STM32F10x.h"
#include "../Drivers/log_driver/log.h"
#include "../utils/cmath.h"

ADC_InitTypeDef ADC_InitStructure;
__IO uint16_t ADC_RegularConvertedValueTab[64];

extern uint8_t battery;
const uint8_t battery_percent_base[10] = {0, 3, 4, 10, 27, 54, 70, 82, 92, 100};
const uint16_t adc_value_base[10] = {2050, 2077, 2105, 2131, 2157, 2183, 2210, 2236, 2263, 2290};
uint8_t get_battery_percent(uint16_t adc_value)
{
    uint8_t i;
    if (adc_value <= adc_value_base[0])
    {
        return battery_percent_base[0];
    }
    if (adc_value >= adc_value_base[9])
    {
        return battery_percent_base[9];
    }
    for (i = 1; i < 10; i++)
    {
        if (adc_value < adc_value_base[i])
        {
            return (battery_percent_base[i - 1] + (adc_value - adc_value_base[i - 1]) * (battery_percent_base[i] - battery_percent_base[i - 1]) / (adc_value_base[i] - adc_value_base[i - 1]));
        }
    }
    return battery_percent_base[0];
}

void RCC_Configuration(void);
void ADC_Start(void)
{
    uint32_t cfg = DMA_MEMORY_INCREMENT;
    cfg |= ((0 << DMA_PRIORITY_POS) & DMA_PRIORITY_MASK) |
           DMA_PERIPHERAL_TO_MEMORY |
           DMA_TRANSFER_COMPLETE_INTERRUPT |
           DMA_PERIPHERAL_DATA_16BIT |
           DMA_MEMORY_DATA_16BIT;

    DMA_ChannelConfigure(DMA1_Channel1,
                         cfg,
                         (uint32_t)(&(ADC1->DR)),
                         (uint32_t)ADC_RegularConvertedValueTab,
                         64);
    DMA_ChannelEnable(DMA1_Channel1);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
void ADC_Initialize(void)
{

    RCC_Configuration();
    GPIO_PortClock(GPIOC, true);
    GPIO_PinConfigure(GPIOC, 1, GPIO_IN_ANALOG, GPIO_MODE_INPUT);
    DMA_ChannelInitialize(1, 1);
    /* ADC1 configuration ------------------------------------------------------*/
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    /* ADC1 regular channels configuration */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_28Cycles5);

    ADC_DMACmd(ADC1, ENABLE);
    /* Enable ADC1 */
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1); //复位校准寄存器
    while (ADC_GetResetCalibrationStatus(ADC1))
        ; //等待校准寄存器复位完成

    ADC_StartCalibration(ADC1); //ADC校准
    while (ADC_GetCalibrationStatus(ADC1))
        ; //等待校准完成
}

void RCC_Configuration(void)
{
    RCC_ADCCLKConfig(RCC_PCLK2_Div4);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,
                           ENABLE);
}

void DMA1_Channel1_Event(uint32_t event)
{
    if (DMA_ChannelTransferItemCount(DMA1_Channel1) != 0)
    {
        return;
    }
    ADC_SoftwareStartConvCmd(ADC1, DISABLE);
    battery = get_battery_percent(average16((uint16_t *)ADC_RegularConvertedValueTab, 64));
    // LOG_printf("ADC_Value:%d",average16((uint16_t *)ADC_RegularConvertedValueTab,64));
}
