/**
 * @file WS2812.c
 * @author LYQ
 * @brief WS2812_driver. SPI_config£¬9MHz£¬CPOL=Low,CPHA=1Edge
 * @version 0.1
 * @date 2025-11-25
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "WS2812.h"

#define LED_NUM 12
#define BIT_PER_LED 24
#define SPI_BIT_PER_CODE 8
#define BUF_SIZE (LED_NUM * BIT_PER_LED)

#define GREEN_BIT_OFFSET 0
#define RED_BIT_OFFSET 8
#define BLUE_BIT_OFFSET 16
// #define WS2812_SPI_1 0b11111100
// #define WS2812_SPI_0 0b11000000
#define WS2812_SPI_1 0xFC
#define WS2812_SPI_0 0xC0
#define RESET_BUF_LENGTH 1000
#define constrain(x, x_min, x_max) x<x_min ? x_min : x> x_max ? x_max : x
void WS2812_Init(WS2812_Group_t *WS2812_Group);
void WS2812_SPI_Send_All(WS2812_Group_t *WS2812_Group);
void WS2812_Fill_TX_BUF(WS2812_Group_t *WS2812_Group);
void WS2812_Set_All_LED_One_Color(WS2812_Group_t *WS2812_Group, WS2812_RGB_info_t *WS2812_RGB);
uint8_t tx_buf[BUF_SIZE];
WS2812_RGB_info_t Group_RGB[LED_NUM];
WS2812_Group_t WS2812_Group = {
    .init = WS2812_Init,
};

void WS2812_Init(WS2812_Group_t *WS2812_Group)
{

    WS2812_Group->pGroup_RGB_info = Group_RGB;
    WS2812_Group->tx_buf = tx_buf;
    WS2812_Group->hspi = &hspi1;
    WS2812_Group->send_all = WS2812_SPI_Send_All;
    WS2812_Group->fill_txbuf = WS2812_Fill_TX_BUF;
    WS2812_Group->set_all_one_color = WS2812_Set_All_LED_One_Color;
}

/**
 * @brief
 */
void WS2812_Fill_TX_BUF(WS2812_Group_t *WS2812_Group)
{

    if (WS2812_Group == NULL || WS2812_Group->tx_buf == NULL || WS2812_Group->pGroup_RGB_info == NULL)
    {
        return;
    }
    // memset(WS2812_Group->tx_buf, 0, BUF_SIZE);

    for (uint16_t i = 0; i < LED_NUM; i++)
    {

        uint8_t *buf_ptr = &(WS2812_Group->tx_buf[i * 24]);

        WS2812_RGB_info_t c = WS2812_Group->pGroup_RGB_info[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (c.green & (1 << (7 - bit)))
            {
                buf_ptr[GREEN_BIT_OFFSET + bit] = WS2812_SPI_1;
            }
            else
            {
                buf_ptr[GREEN_BIT_OFFSET + bit] = WS2812_SPI_0;
            }
        }

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (c.red & (1 << (7 - bit)))
            {
                buf_ptr[RED_BIT_OFFSET + bit] = WS2812_SPI_1;
            }
            else
            {
                buf_ptr[RED_BIT_OFFSET + bit] = WS2812_SPI_0;
            }
        }

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (c.blue & (1 << (7 - bit)))
            {
                buf_ptr[BLUE_BIT_OFFSET + bit] = WS2812_SPI_1;
            }
            else
            {
                buf_ptr[BLUE_BIT_OFFSET + bit] = WS2812_SPI_0;
            }
        }
    }
}

/**
 * @brief
 */
void WS2812_Set_All_LED_One_Color(WS2812_Group_t *WS2812_Group, WS2812_RGB_info_t *WS2812_RGB)
{
    for (uint16_t i = 0; i < LED_NUM; i++)
    {
        WS2812_Group->pGroup_RGB_info[i] = *WS2812_RGB;
    }
}
void WS2812_SPI_Send_All(WS2812_Group_t *WS2812_Group)
{
    __disable_irq();
    HAL_SPI_Transmit(WS2812_Group->hspi, WS2812_Group->tx_buf, BUF_SIZE, HAL_MAX_DELAY);
    uint8_t reset_buf[RESET_BUF_LENGTH] = {0};
    HAL_SPI_Transmit(WS2812_Group->hspi, reset_buf, RESET_BUF_LENGTH, HAL_MAX_DELAY);
    __enable_irq();
}
