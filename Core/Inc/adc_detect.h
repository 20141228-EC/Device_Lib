#ifndef __ADC_DETECT_H
#define __ADC_DETECT_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "ws2812.h"

/* ================== 基本参数 ================== */
#define CHANNEL_NUM        4U
#define ADC_RESOLUTION     4096U
#define ADC_REF_VOLTAGE    3.3f

typedef enum {
    BALL_NONE = 0,
    BALL_SMALL,
    BALL_LARGE
} BALL;

/* ================== 手动可调配置 ==================
 * gain[ch]：通道增益（用于把不同通道响应“拉齐”），默认 1.0
 * small_v_min/max：小弹丸差分电压范围（单位 V）
 * large_v_min：大弹丸差分电压阈值（单位 V）
 * large_ch_min：判定大弹丸需要满足 large_v_min 的通道数量
 * debounce：消抖（ms）
 * interval：最小触发间隔（ms）——现在会尽量只作为“硬保护”，建议调小
 */
typedef struct {
    float    gain[CHANNEL_NUM];

    float    small_v_min;
    float    small_v_max;

    float    large_v_min;
    uint8_t  large_ch_min;

    uint32_t small_debounce_ms;
    uint32_t large_debounce_ms;

    uint32_t small_interval_ms;
    uint32_t large_interval_ms;

    uint16_t baseline_sample_cnt;   // 上电 baseline 采样次数
} ADC_Detect_Config_t;

typedef struct {
    uint16_t baseline[CHANNEL_NUM];   // 上电自动采的零点
    uint8_t  baseline_ready;
} ADC_Base_t;

typedef struct {
    BALL     valid_ball;     // 兼容保留：用于调试/最后一次类型
    uint32_t last_trigger;   // 最近一次确认命中的时间戳(ms)
} ADC_Detect_State_t;

extern uint16_t adc_dma_buf[CHANNEL_NUM];

extern ADC_Detect_Config_t adc_detect_cfg;
extern ADC_Base_t         adc_base;
extern ADC_Detect_State_t adc_detect_state;

void adc_detect_init(void);
void adc_detect_process(WS2812_Device_t *dev);

/* 兼容旧接口：从队列里取“下一发命中类型”（取一次弹出一次） */
BALL adc_detect_get_valid_ball(void);

/* 新增：直接拿命中计数（用于算血量最稳） */
uint32_t adc_detect_get_hit_total(void);
uint32_t adc_detect_get_hit_small(void);
uint32_t adc_detect_get_hit_large(void);
void     adc_detect_clear_hit_counts(void);

/* 调参辅助：读取当前每通道“差分电压”(V)，用于现场观测/打印 */
void adc_detect_get_vdiff(float out_vdiff[CHANNEL_NUM]);

/* ws2812 工具 */
void ws2812_set_all(WS2812_Device_t *dev, uint8_t r, uint8_t g, uint8_t b);
void ws2812_show(WS2812_Device_t *dev);

/* 受击闪烁：现在是“非阻塞启动”，闪烁在 adc_detect_process 内部自动推进 */
void ws2812_hit_blink_blue_twice(WS2812_Device_t *dev);

#endif /* __ADC_DETECT_H */
