#ifndef __WS2812_H
#define __WS2812_H


#include "stm32f1xx_hal.h"
#include "spi.h"
#include "string.h"

__packed typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} WS2812_RGB_info_t;

typedef struct WS2812_Group_Struct{
	SPI_HandleTypeDef* hspi;  
	WS2812_RGB_info_t*  pGroup_RGB_info;
	uint8_t* tx_buf;
	void(*init)(struct WS2812_Group_Struct* WS2812_Group);
	void(*set_all_one_color)(struct WS2812_Group_Struct* WS2812_Group,WS2812_RGB_info_t* WS2812_RGB);
	void(*fill_txbuf)(struct WS2812_Group_Struct* WS2812_Group);
	void(*send_all)(struct WS2812_Group_Struct* WS2812_Group);
}WS2812_Group_t;

extern WS2812_Group_t WS2812_Group;

#endif


