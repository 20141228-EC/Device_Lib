/**
 *@file uart_screen_protocol.c
 *@brief 串口触摸屏通信
 *@version 1.0
 *@date 2025-11-18 
 */
#include "car.h"
//#include "uart_screen_protocol.h"
#include "stdbool.h"
#include "string.h"
#include "usart.h"
#include "crc.h"
#include "tuner.h"
#include "semphr.h"
#include "remote.h"
#include "launcher.h"
#include "axis.h"
#include "force.h"
#include <stdio.h>
#include <stdint.h>
#include "uartscreen_protocol.h"  

#define POINT_COUNT   2   //虚拟浮点数位数

/**
  * @brief  计算CRC-16/Modbus校验值,陶晶驰串口屏使用CRC-16/Modbus校验。
  * @param  pData: 数据指针
  * @param  Length: 数据长度
  * @retval CRC16校验值
  */
uint16_t CRC16_Modbus(uint8_t *pData, uint16_t Length)
{
    uint16_t crc = 0xFFFF;  // 初始值
    uint16_t i, j;

    if (pData == NULL) {
        return 0;
    }

    for (j = 0; j < Length; j++) {
        crc ^= (uint16_t)pData[j];  // 与数据异或

        for (i = 0; i < 8; i++) {
            if (crc & 0x0001) {     // 如果最低位为1
                crc = (crc >> 1) ^ 0xA001;  // 右移并与反转多项式异或
            } else {
                crc = crc >> 1;     // 否则直接右移
            }
        }
    }
    
    return crc;
}

tjc_packet_header_t tjc_packet_header = {0};
tjc_rx_info_t tjc_rx_info={0};
tjc_tx_info_t tjc_tx_info={0};
tjc_finaldata_t tjc_final_data={0};

tjc_t tjc={
	.tjc_packet_header =&tjc_packet_header,
	.tjc_rx_info =&tjc_rx_info,
	.tjc_tx_info =&tjc_tx_info,
	.tjc_finaldata =&tjc_final_data,
	.CRC16 = 0,
};

/*------------------------接收函数-----------------------------------------*/

#define TJC_PACKET_HEAD  0xA5
#define TJC_PACKET_ID    0x01
#define TJC_PROTOCOL_ID  0x00E0
#define TJC_DATA_LENGTH  (sizeof(tjc_packet_header_t)+sizeof(tjc_rx_info_t))
#define TJC_HEAD_LENGTH  4

/**
 *@brief 接收数据函数
 */
void Tjc_Rx_Data(uint8_t *rxBuf)
{	
	uint16_t rx_CRC16=CRC16_Modbus(rxBuf,TJC_DATA_LENGTH - 2);      //接收到数据计算出来的crc
	
	memcpy(tjc.tjc_packet_header, rxBuf, sizeof(tjc_packet_header_t));
	if(!(tjc.tjc_packet_header->SOF==TJC_PACKET_HEAD && tjc.tjc_packet_header->pack_id==TJC_PACKET_ID && tjc.tjc_packet_header->protocol_id==TJC_PROTOCOL_ID))
	{
		return;
	}
	else
	{
		memcpy(tjc.tjc_rx_info,rxBuf+TJC_HEAD_LENGTH,sizeof(tjc_rx_info_t));
		
		if(tjc.tjc_rx_info->tjc_CRC16 == rx_CRC16)    //tjc_rx_info_temp.tjc_CRC16:接收数据自带的crc
		{
			uint16_t divitioon =1;       //根据小数位数缩小倍数
			for(int i=0;i<POINT_COUNT;i++)
			{
				divitioon*=10;
			}
			tjc.tjc_finaldata->axis_all_offset = (float)tjc.tjc_rx_info->axis_all_offset/divitioon;
			tjc.tjc_finaldata->axis_first_offset =(float)tjc.tjc_rx_info->axis_first_offset/divitioon;
			tjc.tjc_finaldata->axis_second_offset =(float)tjc.tjc_rx_info->axis_second_offset/divitioon;
			tjc.tjc_finaldata->axis_third_offset =(float)tjc.tjc_rx_info->axis_third_offset/divitioon;
			tjc.tjc_finaldata->axis_fourth_offset =(float)tjc.tjc_rx_info->axis_fourth_offset/divitioon;
			tjc.tjc_finaldata->force_all_offset =(float)tjc.tjc_rx_info->force_all_offset/divitioon;
			tjc.tjc_finaldata->force_first_offset =(float)tjc.tjc_rx_info->force_first_offset/divitioon;
			tjc.tjc_finaldata->force_second_offset =(float)tjc.tjc_rx_info->force_second_offset/divitioon;				
			tjc.tjc_finaldata->force_third_offset =(float)tjc.tjc_rx_info->force_third_offset/divitioon;
			tjc.tjc_finaldata->force_fourth_offset =(float)tjc.tjc_rx_info->force_fourth_offset/divitioon;
//			if (car.car_ctrl_mode == CAR_SLEEP_MODE)  //关控条件下写入flash（写操作需要一定时间，建议在关控情况下进行）
//            {
                Tuner_UpdateTask_Start(); // 启动更新任务
//            }
		}
		else
		{
			memset(tjc.tjc_packet_header,0,sizeof(tjc_packet_header_t));
			memset(tjc.tjc_rx_info,0,sizeof(tjc_rx_info_t));
			return;
		}
	}
	
}


/*-----------------------发送函数---------------------------------*/
  
// TJC指令结束符：3个0xFF
#define TJC_END_FF "\xFF\xFF\xFF"

// 设置文本
void TJC_SetText(const char* obj_name, const char* text)
{
    char cmd[64];
    sprintf(cmd, "%s.txt=\"%s\"\xff\xff\xff", obj_name, text);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 100);
}

// 设置数值
void TJC_SetValue(const char* obj_name, int value)
{
    char cmd[32];
    sprintf(cmd, "%s.val=%d\xff\xff\xff", obj_name, value);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 100);
}

// 设置颜色
void TJC_SetColor(const char* obj_name, uint16_t color)
{
    char cmd[32];
    sprintf(cmd, "%s.pco=%d\xff\xff\xff", obj_name, color);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 100);
}

// 切换页面
void TJC_ChangePage(uint8_t page_id)
{
    char cmd[16];
    sprintf(cmd, "page %d\xff\xff\xff", page_id);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 100);
}

// 使能/禁用控件
void TJC_SetVisible(const char* obj_name, uint8_t enable)
{
    char cmd[32];
    sprintf(cmd, "vis %s,%d\xff\xff\xff", obj_name, enable);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 100);
}
