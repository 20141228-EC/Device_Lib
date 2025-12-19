/**
 * @file   BRT_encoder.c
 * @brief  辅助模块-外置机械编码器相关
 */
/*Include-----------------------------------------------------------*/
#include "BRT_encoder.h"
/*Privated Function----------------------------------------------------*/
static void BRT_encoder_update(BRT_encoder_t* BRT_encoder,uint8_t* rxBuf);
static uint32_t get_encoder_angle(uint8_t* rxBuf);
static float encoder_angle_to_angle(uint32_t encoder_angle);
/**
 * @brief  解析角度
 * @note   在can的中断回调中调用
 */
static void BRT_encoder_update(BRT_encoder_t* BRT_encoder,uint8_t* rxBuf)
{
	BRT_encoder->encoder_angle=get_encoder_angle(rxBuf);
	BRT_encoder->angle=encoder_angle_to_angle(BRT_encoder->encoder_angle);
}	
/**
 * @brief  初始化
 */
void BRT_encoder_init(BRT_encoder_t* BRT_encoder)
{
	BRT_encoder->update=BRT_encoder_update;
}
/*--------------------------------------工具函数----------------------------------------------------*/
/**
 * @brief  获取编码角度值
 */
static uint32_t get_encoder_angle(uint8_t* rxBuf)
{
	uint32_t encoder_angle;
	encoder_angle=( (rxBuf[6]<<8 | rxBuf[5])<<16 | (rxBuf[4]<<8 | rxBuf[3]) );
	return encoder_angle;
}
/**
 * @brief  角度单位转换为 度
 */
static float encoder_angle_to_angle(uint32_t encoder_angle)
{
	float angle;
	angle=encoder_angle*360.0/BRT_resolution;
	return angle;
}
