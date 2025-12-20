#ifndef __WS2812_H
#define __WS2812_H

#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdint.h>

extern volatile uint8_t ws2812_pause_flag;

/*
 * 重要：以下编码是给你当前 SPI=4.5MHz 用的（SPI1 72MHz /16 = 4.5MHz）
 * 1个WS位=1个SPI字节(8bit)，所以 WS位周期 = 8/4.5MHz = 1.777us
 * CODE0：1个“1”后面全0 -> TH≈0.222us（更不容易把0误判成1）
 * CODE1：3个“1”后面全0 -> TH≈0.666us
 */
#define WS2812_CODE0 0x80  // 1000 0000
#define WS2812_CODE1 0xE0  // 1110 0000

#define WS2812_MAX_LED             12
#define WS2812_LOGIC_LED_NUM       12

#define WS2812_SPI_BYTES_PER_LED   24

// 80也能用，但建议更大一点更稳：>200us 低电平更保险
#define WS2812_RESET_BYTES         120

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} WS2812_RGB_t;

typedef struct {
    SPI_HandleTypeDef *hspi;
    uint16_t led_num;
    WS2812_RGB_t led_buf[WS2812_MAX_LED];
    uint8_t tx_buf[WS2812_MAX_LED * WS2812_SPI_BYTES_PER_LED + WS2812_RESET_BYTES];
    volatile uint8_t dma_done;
} WS2812_Device_t;

void ws2812_init(WS2812_Device_t *dev, SPI_HandleTypeDef *hspi, uint16_t led_num);
void ws2812_set_color(WS2812_Device_t *dev, uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void ws2812_fill(WS2812_Device_t *dev, uint8_t r, uint8_t g, uint8_t b);
void ws2812_encode(WS2812_Device_t *dev);
HAL_StatusTypeDef ws2812_send(WS2812_Device_t *dev, uint32_t timeout_ms);
void ws2812_spi_tx_cplt_isr(WS2812_Device_t *dev);

#endif
