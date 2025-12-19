/**
 * @file  magnetic_encoder.c
 * @brief 复位模块-磁编码器
 */
#include "magnetic_encoder.h"
/*Privated Function------------------------------------------------------------*/
static void magnetic_encoder_update(magnetic_encoder_t* magnetic_encoder,uint8_t* rxBuf);
static void magnetic_encoder_heart_beat(magnetic_encoder_t* magnetic_encoder);
static uint16_t get_encoder_angle(uint8_t* rxBuf);
static float encoder_angle_to_angle(uint16_t encoder_angle);
/**
 * @brief  接收磁编码器角度
 * @note   在主循环中调用,rxBuf是一个 data[2] 即可
 */
static void magnetic_encoder_update(magnetic_encoder_t* magnetic_encoder,uint8_t* rxBuf)
{
	magnetic_encoder->encoder_angle=get_encoder_angle(rxBuf);
	magnetic_encoder->angle=encoder_angle_to_angle(magnetic_encoder->encoder_angle);
	
	magnetic_encoder->magnetic_encoder_state.offline_cnt=0;
}
/**
 * @brief 心跳
 */
static void magnetic_encoder_heart_beat(magnetic_encoder_t* magnetic_encoder)
{
	magnetic_encoder->magnetic_encoder_state.offline_cnt++;
	if(magnetic_encoder->magnetic_encoder_state.offline_cnt>magnetic_encoder->magnetic_encoder_state.offline_cnt_max)
	{
		magnetic_encoder->magnetic_encoder_state.device_status=Device_OFFLINE;
		magnetic_encoder->magnetic_encoder_state.offline_cnt=magnetic_encoder->magnetic_encoder_state.offline_cnt_max; //防止数据溢出
	}else //实际上在线
	{
		if(magnetic_encoder->magnetic_encoder_state.device_status==Device_OFFLINE)
		{
			magnetic_encoder->magnetic_encoder_state.device_status=Device_ONLINE;
		}
	}
}
/**
 * @brief  初始化
 */
void magnetic_encoder_init(magnetic_encoder_t* magnetic_encoder)
{
	magnetic_encoder->update=magnetic_encoder_update;
	magnetic_encoder->heart_beat=magnetic_encoder_heart_beat;
	
	magnetic_encoder->magnetic_encoder_state.offline_cnt_max=60;
	magnetic_encoder->magnetic_encoder_state.device_status=Device_OFFLINE;
	magnetic_encoder->magnetic_encoder_state.offline_cnt=magnetic_encoder->magnetic_encoder_state.offline_cnt_max; //避免开机未在线时，被误判为在线
}
/*------------------------------------工具函数-----------------------------------------------------*/
/**
 * @brief  获取角度编码值
 */
static uint16_t get_encoder_angle(uint8_t* rxBuf)
{
	uint16_t encoder_angle;
	encoder_angle= ( (rxBuf[0]<<6) | (rxBuf[1]>>2) );
	return encoder_angle;
}
/**
 * @brief  把角度单位转化为 度
 */
static float encoder_angle_to_angle(uint16_t encoder_angle)
{
	float angle;
	angle=encoder_angle/MAGNETIC_RESOLUTION*360.0;
	return angle;
}
