#include "can_receive.h"
volatile uint8_t color_update_flag = 0;  // 颜色更新请求标志
volatile uint8_t target_color = COLOR_OFF; // 目标颜色缓存
volatile uint8_t reset_pending     = 0;
CAN_RxHeaderTypeDef   can_rx_header;
uint8_t               can_rx_data[CAN_DATA_LEN] = {0};  // 初始化为0，避免脏数据

CAN_TxHeaderTypeDef   can_tx_header;
uint8_t               can_tx_data[CAN_DATA_LEN] = {0};
uint32_t              can_tx_mailbox;
float c = 0;
volatile uint8_t g_armor_r = 0;
volatile uint8_t g_armor_g = 0;
volatile uint8_t g_armor_b = 255;
// CAN接收滤波配置（只接收中控0x123的帧，过滤其他干扰）
HAL_StatusTypeDef CAN_Init(CAN_HandleTypeDef *hcan)
{
    /* 1. 过滤器配置：仅接收ID=0x123的标准帧 */
    CAN_FilterTypeDef  sFilterConfig = {0};

    sFilterConfig.FilterBank = CAN_FILTER_BANK;                     // 过滤器编号
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;               // 掩码模式（精准匹配）
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;              // 32位过滤宽度
    sFilterConfig.FilterIdHigh = (CAN_COLOR_ID << 5);               // 接收ID=0x123（标准ID左移5位对齐）
    sFilterConfig.FilterIdLow = 0x0000;                             // 低16位无数据
    sFilterConfig.FilterMaskIdHigh = (0x7FF << 5);                  // 11位标准ID全匹配掩码
    sFilterConfig.FilterMaskIdLow = 0x0000;                         // 低16位掩码
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO;               // 匹配消息存入指定FIFO
    sFilterConfig.FilterActivation = ENABLE;                        // 使能过滤器
    sFilterConfig.SlaveStartFilterBank = 14;                        // 从CAN过滤器起始编号（单CAN无意义）

    if (HAL_CAN_ConfigFilter(hcan, &sFilterConfig) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 2. 发送消息头配置（ID=0x124，标准数据帧） */
    can_tx_header.StdId = CAN_PROJECTILE_HIT_ID;  // 发送ID=0x124
    can_tx_header.ExtId = 0x00;                   // 禁用扩展ID
    can_tx_header.RTR = CAN_RTR_DATA;             // 数据帧（非远程帧）
    can_tx_header.IDE = CAN_ID_STD;               // 标准ID格式
    can_tx_header.DLC = CAN_DATA_LEN;             // 数据长度=8字节
    can_tx_header.TransmitGlobalTime = DISABLE;   // 禁用发送时间戳

    return HAL_OK;
}




void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  
	
	if (hcan->Instance == CAN1)  
    {
        // 读取接收消息（存入全局变量can_rx_header/can_rx_data）
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO, &can_rx_header, can_rx_data) == HAL_OK)
        {
            // 仅处理ID=0x123的消息（双重校验，避免过滤器配置错误）
            if (can_rx_header.StdId == CAN_COLOR_ID && can_rx_header.IDE == CAN_ID_STD)
            {
                CAN_Receive_Color_Data_Handle();  // 调用数据解析函数
            }
        }
    }
	
}

void CAN_Receive_Color_Data_Handle(void)
{
	
	
	uint8_t recv_data = can_rx_data[0];
	if (recv_data == SOFT_RESET)
    {
        reset_pending = 1;
        return;
    }
	// 校验合法颜色（避免无效值）
	if (recv_data == COLOR_OFF || recv_data == COLOR_RED || 
			recv_data == COLOR_GREEN || recv_data == COLOR_BLUE)
	{
			target_color = recv_data;
	}
	else
	{
			target_color = COLOR_RED; // 非法颜色默认红色
	}
	color_update_flag = 1; // 仅设置标志位，不直接执行颜色更新
		
}
void armor_set_color(uint8_t color) {
     uint8_t r = 0, g = 0, b = 0;

    // 第一步：根据指令计算RGB
    switch (color) {
        case COLOR_OFF:
            r = 0;   g = 0;   b = 0;
            break;
        case COLOR_RED:
            r = 255; g = 0;   b = 0;
            break;
        case COLOR_GREEN:
            r = 0;   g = 255; b = 0;
            break;
        case COLOR_BLUE:
            r = 0;   g = 0;   b = 255;
            break;
        default:
            // 未知指令默认青色（方便调试）
            r = 0;   g = 255; b = 255;
            break;
    }

    // 关键：更新当前底色记录（供受击闪烁后恢复）
    g_armor_r = r;
    g_armor_g = g;
    g_armor_b = b;

    // 第二步：设置所有灯珠颜色
    for (uint16_t i = 0; i < ws2812_dev.led_num; i++) {
        ws2812_set_color(&ws2812_dev, i, r, g, b);
    }

    // 第二步：所有灯珠颜色设置完成后，统一编码+发送（关键！）
    ws2812_encode(&ws2812_dev);
    ws2812_send(&ws2812_dev,50);
}

HAL_StatusTypeDef CAN_Send_Projectile_Hit(CAN_HandleTypeDef *hcan, uint8_t projectile_type)
{
    
    // 构造发送数据：第0字节=弹丸类型，其余补0（符合8字节数据帧要求）
    memset(can_tx_data, 0x00, CAN_DATA_LEN);  // 所有字节先清0
    can_tx_data[0] = projectile_type;         // 第0字节存储弹丸类型（0x04/0x09）

    // 发送CAN消息（自动选择空闲邮箱）
    return HAL_CAN_AddTxMessage(hcan, &can_tx_header, can_tx_data, &can_tx_mailbox);
}



void ProjectileHit_Process(CAN_HandleTypeDef *hcan) {
    BALL ball_type = adc_detect_get_valid_ball();
		
    // 根据类型发送CAN消息
    switch (ball_type) {
        case BALL_SMALL:
            // 发送小弹丸标识：0x04
						CAN_Send_Projectile_Hit(hcan, PROJECTILE_SMALL);
            break;
        case BALL_LARGE:
            // 发送大弹丸标识：0x09
            CAN_Send_Projectile_Hit(hcan, PROJECTILE_LARGE);
            break;
        case BALL_NONE:
        default:
            // 无有效击打，不发送CAN
            break;
    }
}
void CAN_Task_Poll(void)
{
   
    if (reset_pending)
    {
        reset_pending = 0;
        Soft_Reset();
    }

    
}

void Soft_Reset(void)
{
    __disable_irq();
    __DSB();
    __ISB();
    NVIC_SystemReset();

    while (1)
    {
        // 正常不会执行到这里
    }
}
