#ifndef CAN_RECEIVE_H
#define CAN_RECEIVE_H

#include "can.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "spi.h"
#include "ws2812.h"
#include "adc_detect.h"


// 颜色编码（与中控端完全一致）
#define COLOR_OFF    0x05
#define COLOR_RED    0x06
#define COLOR_GREEN  0x07
#define COLOR_BLUE   0x08
// 装甲板发送给主控的CAN ID
#define CAN_COLOR_ID      0x123
#define CAN_PROJECTILE_HIT_ID 0x124 
// 弹丸类型定义
#define PROJECTILE_SMALL 0x04  // 小弹丸
#define PROJECTILE_LARGE 0x09  // 大弹丸
#define SOFT_RESET 0x01
#define CAN_FILTER_BANK       0              // 过滤器编号
#define CAN_RX_FIFO           CAN_RX_FIFO0   // 接收FIFO（CAN_RX_FIFO0/CAN_RX_FIFO1）
#define CAN_DATA_LEN          8              // CAN数据长度（字节，0~8）
#define CAN_TIMEOUT_MS        100            // 发送超时时间（ms）
//接收相关
extern CAN_RxHeaderTypeDef   can_rx_header;  // 接收消息头
extern uint8_t               can_rx_data[CAN_DATA_LEN];  // 接收数据缓存
// 发送相关
extern CAN_TxHeaderTypeDef   can_tx_header;  // 发送消息头（击中反馈专用）
extern uint8_t               can_tx_data[CAN_DATA_LEN];  // 发送数据缓存
extern uint32_t              can_tx_mailbox; // 发送邮箱编号
extern WS2812_Device_t ws2812_dev; 
extern volatile uint8_t color_update_flag;
extern volatile uint8_t target_color;
HAL_StatusTypeDef CAN_Init(CAN_HandleTypeDef *hcan);                // CAN初始化
void CAN_Receive_Color_Data_Handle(void);
HAL_StatusTypeDef CAN_Send_Projectile_Hit(CAN_HandleTypeDef *hcan,uint8_t projectile_type);
void armor_set_color(uint8_t color);
void ProjectileHit_Process(CAN_HandleTypeDef *hcan);
void CAN_Task_Poll(void);
void Soft_Reset(void);
#endif
