#ifndef __BRT_ENCODER_H
#define __BRT_ENCODER_H
/*Include--------------------------*/
#include "device_related.h"
/*Exported Macro---------------------*/
//单圈分辨率           2的12次方
#define BRT_resolution 4096.0 
typedef struct BRT_encoder_struct_t{
	uint32_t encoder_angle;
	float angle; //单位：度
//	Device_state_t BRT_state;
	
	void (*update)(struct BRT_encoder_struct_t* BRT_encoder,uint8_t* rxBuf);
	void (*init)(struct BRT_encoder_struct_t* BRT_encoder);
}BRT_encoder_t;
/*Exported Functions------------------*/
void BRT_encoder_init(BRT_encoder_t* BRT_encoder);
#endif
