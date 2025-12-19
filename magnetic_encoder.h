#ifndef __MAGNETIC_ENCODER_H
#define __MAGNETIC_ENCODER_H
#include "device_related.h"
/*Exported Macro-------------------------------------------------*/
#define MAGNETIC_RESOLUTION 16384.0
typedef struct magnetic_encoder_struct_t{
	uint16_t encoder_angle; //定义变量时需先初始化为0（因为实际 编码器角度 只占了14位）
	float angle; //单位 度
	Device_state_t magnetic_encoder_state; 
	
	uint8_t ex_rxBuf[2]; //外部iic接口
	
	void (*update)(struct magnetic_encoder_struct_t* magnetic_encoder,uint8_t* rxBuf);
	void (*heart_beat)(struct magnetic_encoder_struct_t* magnetic_encoder);
	void (*init)(struct magnetic_encoder_struct_t* magnetic_encoder);
}magnetic_encoder_t;
/*Exported Functions--------------------------------*/
void magnetic_encoder_init(magnetic_encoder_t* magnetic_encoder);
#endif
