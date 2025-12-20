/**
 *@file uart_screen_protocol.h
 *@brief 陶晶驰串口屏通信协议
 *@Version 1.0
 *@date 2025-11-20
 */#ifndef _UARTSCREEN_PROTOCOL_H
#define _UARTSCREEN_PROTOCOL_H
#include "config.h"

 
 
 
 /**
  *@biref 陶晶驰数据包包头
  */
 typedef struct 
 {
	 uint8_t SOF;       //A5 
	 uint8_t pack_id;   //01
	 uint16_t protocol_id;     //00E0
 }__attribute__((packed)) tjc_packet_header_t;
 
 
 /**
  *@biref 接收数据包
  *@id    01
  *
  */
 
typedef struct
{
	int32_t force_all_offset;
	int32_t axis_all_offset;
	 
	int32_t force_first_offset; // 力量第一发位置偏移
	int32_t force_second_offset; // 力量第二发位置偏移
	int32_t force_third_offset; // 力量第三发位置偏移
	int32_t force_fourth_offset; // 力量第四发位置偏移
	
	int32_t axis_first_offset; // 轴第一发位置偏移
	int32_t axis_second_offset; // 轴第二发位置偏移
	int32_t axis_third_offset; // 轴第三发位置偏移
	int32_t axis_fourth_offset; // 轴第四发位置偏移	
	
	uint16_t tjc_CRC16;
	 
}__attribute__((packed)) tjc_rx_info_t;
 

typedef struct
{
	float force_all_offset;
	float axis_all_offset;
	 
	float force_first_offset; // 力量第一发位置偏移
	float force_second_offset; // 力量第二发位置偏移
	float force_third_offset; // 力量第三发位置偏移
	float force_fourth_offset; // 力量第四发位置偏移
	
	float axis_first_offset; // 轴第一发位置偏移
	float axis_second_offset; // 轴第二发位置偏移
	float axis_third_offset; // 轴第三发位置偏移
	float axis_fourth_offset; // 轴第四发位置偏移	
	 
}__attribute__((packed)) tjc_finaldata_t;

/**
 *@brief 发送数据包
 */
 
typedef struct
{
	int32_t force_offest_measure;
	int32_t axis_offest_measure;
	
}__attribute__((packed)) tjc_tx_info_t;
 
typedef struct
{
	tjc_packet_header_t* tjc_packet_header;
	tjc_rx_info_t* tjc_rx_info;
	tjc_tx_info_t* tjc_tx_info;
	int16_t CRC16;
	tjc_finaldata_t *tjc_finaldata;
}tjc_t;

extern tjc_t tjc;
void Tjc_Rx_Data(uint8_t *rxBuf);
void Tic_Tx_Data(void);

void TJC_SetText(const char* obj_name, const char* text);
void TJC_SetValue(const char* obj_name, int value);
void TJC_SetColor(const char* obj_name, uint16_t color);
void TJC_ChangePage(uint8_t page_id);
void TJC_SetVisible(const char* obj_name, uint8_t enable);
#endif