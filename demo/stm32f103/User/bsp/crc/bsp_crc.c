/**
  ******************************************************************************
  * @file    bsp_ds18b20.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   CRC�������
  ******************************************************************************
  */
#include "include.h"
//#include "./crc/bsp_crc.h"

__IO uint32_t CRCValue = 0;		 // ���ڴ�Ų�����CRCУ��ֵ

/*
 * ��������CRC_Config
 * ����  ��ʹ��CRCʱ��
 * ����  ����
 * ���  ����
 * ����  : �ⲿ����
 */
void CRC_Config(void)
{
	/* Enable CRC clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
}

