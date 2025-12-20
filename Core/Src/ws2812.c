#include "ws2812.h"

volatile uint8_t ws2812_pause_flag = 0;

static void ws2812_delay_us(uint32_t us)
{
    // 粗略延时即可（只用于reset间隔，不用于波形）
    uint32_t ticks = (SystemCoreClock / 1000000U) * us;
    while (ticks--) { __NOP(); }
}

static void ws2812_reset_blocking(WS2812_Device_t *dev)
{
    // 发送全0，让DIN保持低电平足够长
    memset(dev->tx_buf, 0x00, WS2812_RESET_BYTES);
    (void)HAL_SPI_Transmit(dev->hspi, dev->tx_buf, WS2812_RESET_BYTES, 50);
    ws2812_delay_us(300);
}

void ws2812_init(WS2812_Device_t *dev, SPI_HandleTypeDef *hspi, uint16_t led_num)
{
    if (!dev || !hspi) return;
    if (led_num == 0 || led_num > WS2812_MAX_LED) return;

    memset(dev, 0, sizeof(*dev));
    dev->hspi = hspi;
    dev->led_num = led_num;

    // 上电后先给WS2812一点稳定时间（电源缓升/上电乱锁存很常见）
    HAL_Delay(10);

    // 先做一次强复位（低电平>50us）
    ws2812_reset_blocking(dev);

    // 连续发几次全灭帧，把上电乱数据“冲掉”
    ws2812_fill(dev, 0, 0, 255);
    for (int i = 0; i < 4; i++) {
        ws2812_encode(dev);
        (void)ws2812_send(dev, 50);
        ws2812_delay_us(300);
    }
}

void ws2812_set_color(WS2812_Device_t *dev, uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!dev) return;
    if (index >= dev->led_num) return;
    dev->led_buf[index].r = r;
    dev->led_buf[index].g = g;
    dev->led_buf[index].b = b;
}

void ws2812_fill(WS2812_Device_t *dev, uint8_t r, uint8_t g, uint8_t b)
{
    if (!dev) return;
    for (uint16_t i = 0; i < dev->led_num; i++) {
        dev->led_buf[i].r = r;
        dev->led_buf[i].g = g;
        dev->led_buf[i].b = b;
    }
}

void ws2812_encode(WS2812_Device_t *dev)
{
    if (!dev) return;

    uint16_t len = (uint16_t)(dev->led_num * WS2812_SPI_BYTES_PER_LED + WS2812_RESET_BYTES);
    memset(dev->tx_buf, 0x00, len);

    uint32_t idx = 0;

    for (uint16_t i = 0; i < dev->led_num; i++) {
        // WS2812是GRB顺序
        uint8_t grb[3] = { dev->led_buf[i].g, dev->led_buf[i].r, dev->led_buf[i].b };

        for (uint8_t c = 0; c < 3; c++) {
            for (int8_t bit = 7; bit >= 0; bit--) {
                dev->tx_buf[idx++] = (grb[c] & (1U << bit)) ? WS2812_CODE1 : WS2812_CODE0;
            }
        }
    }

    // reset尾巴已经在memset里清0了，这里不用再写也行
    // for (uint16_t k = 0; k < WS2812_RESET_BYTES; k++) dev->tx_buf[idx++] = 0x00;
}

HAL_StatusTypeDef ws2812_send(WS2812_Device_t *dev, uint32_t timeout_ms)
{
    if (ws2812_pause_flag) return HAL_BUSY;
    if (!dev || !dev->hspi) return HAL_ERROR;

    uint32_t t0 = HAL_GetTick();
    while (HAL_SPI_GetState(dev->hspi) != HAL_SPI_STATE_READY) {
        if ((HAL_GetTick() - t0) > timeout_ms) return HAL_TIMEOUT;
    }

    dev->dma_done = 0;

    uint16_t len = (uint16_t)(dev->led_num * WS2812_SPI_BYTES_PER_LED + WS2812_RESET_BYTES);
    HAL_StatusTypeDef st = HAL_SPI_Transmit_DMA(dev->hspi, dev->tx_buf, len);
    if (st != HAL_OK) return st;

    t0 = HAL_GetTick();
    while (!dev->dma_done) {
        if ((HAL_GetTick() - t0) > timeout_ms) return HAL_TIMEOUT;
    }

    // 关键：DMA完成≠SPI移位完成，必须等BSY清零
    t0 = HAL_GetTick();
    while (__HAL_SPI_GET_FLAG(dev->hspi, SPI_FLAG_BSY)) {
        if ((HAL_GetTick() - t0) > timeout_ms) return HAL_TIMEOUT;
    }

    ws2812_delay_us(200); // 给reset留点余量
    return HAL_OK;
}

void ws2812_spi_tx_cplt_isr(WS2812_Device_t *dev)
{
    if (dev) dev->dma_done = 1;
}
