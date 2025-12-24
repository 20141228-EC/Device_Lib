#include "adc_detect.h"
extern volatile uint8_t g_armor_r, g_armor_g, g_armor_b;
/* ================== 参数 ================== */
#define HIT_BLINK_COUNT   2U
#define HIT_ON_MS         40U
#define HIT_OFF_MS        40U
/* 浮点基线用于 IIR 跟随：
    - 使用浮点可以避免小幅抖动在整数运算中的舍入误差
    - `baseline_f` 保存当前的浮点基线估计，`baseline_f_inited` 标志是否已被第一次初始化
*/
static float baseline_f[CHANNEL_NUM];   // 浮点基线缓冲（与 adc_base.baseline 对应）
static uint8_t baseline_f_inited = 0;   // 0: 未初始化，1: 已初始化
ADC_Detect_Config_t adc_detect_cfg = {
    .gain = { 1.0f, 1.0f, 1.0f, 1.0f },

    /* 根据实际差分电压来调 */
    .small_v_min = 0.6f,
    .small_v_max = 1.6f,
    .large_v_min = 1.6f,
    .large_ch_min = 1U,

    .small_debounce_ms = 1U,
    .large_debounce_ms = 1U,

    .small_interval_ms = 40U,
    .large_interval_ms = 500U,

    .baseline_sample_cnt = 100U
};

ADC_Base_t adc_base = {0};

ADC_Detect_State_t adc_detect_state = {
    .valid_ball = BALL_NONE,
    .last_trigger = 0U
};

/* ================== 工具函数 ================== */

void ws2812_set_all(WS2812_Device_t *dev, uint8_t r, uint8_t g, uint8_t b)
{
    if (dev == NULL) return;
    for (uint16_t i = 0; i < dev->led_num; i++) {
        ws2812_set_color(dev, i, r, g, b);
    }
}

void ws2812_show(WS2812_Device_t *dev)
{
    if (dev == NULL) return;
    ws2812_encode(dev);
    ws2812_send(dev,50);
}

void ws2812_hit_blink_blue_twice(WS2812_Device_t *dev)
{
    if (dev == NULL) return;

    // 保存受击前颜色快照
    uint8_t save_r = g_armor_r;
    uint8_t save_g = g_armor_g;
    uint8_t save_b = g_armor_b;

    /* 如果在别处用 ws2812_pause_flag 避免影响 ADC，
    这里保存并临时放行显示（否则 send() 会直接 return） */
    uint8_t old_pause = ws2812_pause_flag;
    ws2812_pause_flag = 0;

    for (uint8_t i = 0; i < HIT_BLINK_COUNT; i++) {
        /* 灭 */
        ws2812_set_all(dev, 0, 0, 0);
        ws2812_show(dev);
        HAL_Delay(HIT_OFF_MS);

        
        ws2812_set_all(dev, save_r, save_g, save_b);
        ws2812_show(dev);
        HAL_Delay(HIT_ON_MS);
    }

    /* 关键：恢复受击前颜色（不是固定蓝） */
    ws2812_set_all(dev, save_r, save_g, save_b);
    ws2812_show(dev);

    ws2812_pause_flag = old_pause;
}

static float adc_to_voltage(uint16_t adc)
{
    return (float)adc * ADC_REF_VOLTAGE / ADC_RESOLUTION;
}

static void capture_baseline(uint16_t cnt)
{
    /*
     * 采样计数为 `cnt`，通过多次读取 ADC DMA 缓冲并累加求平均。
     */
    uint32_t sum[CHANNEL_NUM] = {0};

    /* 等待短暂时间让 ADC/DMA 稳定（例如刚启动 ADC 后需时间） */
    HAL_Delay(20);

    /* 连续读取 cnt 次，每次把各通道当前的 ADC 值累加到 sum */
    for (uint16_t i = 0; i < cnt; i++) {
        for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
            /* adc_dma_buf[ch]：来自 DMA 的最新 ADC 原始码 */
            sum[ch] += adc_dma_buf[ch];
        }
        /* 每次读之间短延时，避免总是读到同一次转换的数据 */
        HAL_Delay(1);
    }

    /* 用累计和除以样本数，得到整数基线值，写入 adc_base.baseline */
    for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
        adc_base.baseline[ch] = (uint16_t)(sum[ch] / cnt);
    }

    /* 标记基线已准备好，后续检测可以使用该基线 */
    adc_base.baseline_ready = 1;
}

void adc_detect_get_vdiff(float out_vdiff[CHANNEL_NUM])
{
    if (!out_vdiff) return;

    for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
        uint16_t base = adc_base.baseline[ch];
        uint16_t cur  = adc_dma_buf[ch];
        uint16_t diff = (cur > base) ? (cur - base) : (base - cur);

        float v = adc_to_voltage(diff);
        out_vdiff[ch] = v * adc_detect_cfg.gain[ch];
    }
}

/* ================== 判定逻辑 ================== */
static bool is_small_by_v(float vdiff[CHANNEL_NUM])
{
    for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
        if (vdiff[ch] >= adc_detect_cfg.small_v_min &&
            vdiff[ch] <= adc_detect_cfg.small_v_max) {
            return true;
        }
    }
    return false;
}

static bool is_large_by_v(float vdiff[CHANNEL_NUM])
{
    uint8_t cnt = 0;
    for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
        if (vdiff[ch] >= adc_detect_cfg.large_v_min) {
            cnt++;
        }
    }
    return (cnt >= adc_detect_cfg.large_ch_min);
}

/* ================== 初始化 ================== */
void adc_detect_init(void)
{
    memset(&adc_detect_state, 0, sizeof(adc_detect_state));
    memset(&adc_base, 0, sizeof(adc_base));

    /* 只做 baseline 自动采样，不做任何“按压校准” */
    if (adc_detect_cfg.baseline_sample_cnt < 10) {
        adc_detect_cfg.baseline_sample_cnt = 10;
    }
    capture_baseline(adc_detect_cfg.baseline_sample_cnt);
}

static void baseline_follow_when_quiet(void)
{
    if (!adc_base.baseline_ready) return;
    /*
     * 防止把刚发生的敲击残波学进基线：只有在距离上次触发有足够恢复时间时才跟随。
     * 这里使用 80ms 的恢复窗（经验值），可根据需要调整。
     */
    uint32_t now = HAL_GetTick();
    if (now - adc_detect_state.last_trigger < 80) return;

    /* 计算当前各通道的差分电压（绝对值） */
    float vdiff[CHANNEL_NUM];
    adc_detect_get_vdiff(vdiff);

    /* 判定是否处于“安静”状态：所有通道的 vdiff 都非常小（例如 < 0.02V）
       这里阈值选择比最小触发阈值要小很多，以避免误将微小触发学进基线 */
    bool quiet = true;
    for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
        if (vdiff[ch] > 0.02f) { quiet = false; break; }
    }
    if (!quiet) return; // 非安静时不更新基线

    /* 如果浮点基线尚未初始化，使用当前整数基线做初始值 */
    if (!baseline_f_inited) {
        for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++)
            baseline_f[ch] = (float)adc_base.baseline[ch];
        baseline_f_inited = 1;
    }

    /* 使用一阶 IIR（指数移动平均）更新浮点基线：
       baseline_f += alpha * (cur - baseline_f)
       alpha 越小跟随越慢（更鲁棒但响应慢），alpha=0.002 是经验值 */
    const float alpha = 0.005f; // 可调范围 0.001 ~ 0.01
    for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
        float cur = (float)adc_dma_buf[ch];
        baseline_f[ch] += alpha * (cur - baseline_f[ch]);

        /* 把浮点基线转换回整数存入主基线数组，+0.5f 用于四舍五入 */
        adc_base.baseline[ch] = (uint16_t)(baseline_f[ch] + 0.5f);
    }
}

/* ================== 主处理（循环调用） ================== */
void adc_detect_process(WS2812_Device_t *dev)
{
    if (dev == NULL) return;

    if (!adc_base.baseline_ready) return;

    uint32_t now = HAL_GetTick();
    float vdiff[CHANNEL_NUM];

    adc_detect_get_vdiff(vdiff);

    bool large = is_large_by_v(vdiff);
    bool small = (!large) && is_small_by_v(vdiff);

    /* 时间型消抖：检测到持续满足阈值一段时间才认为有效 */
    static uint32_t debounce_ts = 0;
    static bool debouncing = false;
    static bool last_was_large = false;
    static bool last_was_small = false;

    if (large || small) {
        if (!debouncing) {
            debouncing = true;
            debounce_ts = now;
            last_was_large = large;
            last_was_small = small;
        } else {
            /* 如果过程中类型变化，重新计时（避免边界抖动） */
            if ((large != last_was_large) || (small != last_was_small)) {
                debounce_ts = now;
                last_was_large = large;
                last_was_small = small;
            }

            /* 触发判断 */
            if (large &&
                (now - debounce_ts >= adc_detect_cfg.large_debounce_ms) &&
                (now - adc_detect_state.last_trigger >= adc_detect_cfg.large_interval_ms)) {

                adc_detect_state.valid_ball = BALL_LARGE;
                adc_detect_state.last_trigger = now;
                debouncing = false;
								ws2812_hit_blink_blue_twice(dev);
            }

            if (small &&
                (now - debounce_ts >= adc_detect_cfg.small_debounce_ms) &&
                (now - adc_detect_state.last_trigger >= adc_detect_cfg.small_interval_ms)) {

                adc_detect_state.valid_ball = BALL_SMALL;
                adc_detect_state.last_trigger = now;
                debouncing = false;
								ws2812_hit_blink_blue_twice(dev);
            }
        }
    } else {
        debouncing = false;
		baseline_follow_when_quiet();
    }
}


/* ================== 读取接口 ================== */
BALL adc_detect_get_valid_ball(void)
{
    BALL b = adc_detect_state.valid_ball;
    adc_detect_state.valid_ball = BALL_NONE;
    return b;
}
