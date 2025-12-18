#include "buzzer.h"

void Buzzer_init(buzzer_t* buzzer);
void Buzzer_Work(buzzer_t* buzzer);
buzzer_t buzzer={
	
	.work=Buzzer_Work,
	.init=Buzzer_init,
};
/* Private macro -------------------------------------------------------------*/
#define constrain(x, min, max) ((x > max) ? max : (x < min ? min : x))	
/* Private function-------------------------------------------------------------*/

/**
 * @brief ?????????
 */
void Buzzer_init(buzzer_t* buzzer)
{
	buzzer->config.max_tim_arr=65535;
	buzzer->config.tim_freq=84000000;
	buzzer->config.min_pwm_duty=0;
	buzzer->config.max_pwm_duty=0.8;
	buzzer->config.max_volume=100.f;
	buzzer->config.min_volume=0.f;

}


/**
 * @brief ???????????????????? ARR ?? max_tim_arr??
 * @return ARR??0~65535??
 */
static uint16_t Buzzer_Calc_Optimal_Presc(buzzer_t* buzzer, float buzzer_freq)
{
    if (buzzer_freq <= 0) {
        return 0;  // ???????????????????
    }

    uint32_t tim_freq = buzzer->config.tim_freq;
    uint16_t max_arr = buzzer->config.max_tim_arr;

    // ????????tim_presc = (tim_freq / (buzzer_freq * (max_arr + 1))) - 1
    // ??? ARR = tim_freq/(buzzer_freq*(tim_presc+1)) -1 ?? max_arr
    float presc_float = (tim_freq / (buzzer_freq * (max_arr + 1))) - 1.0f;

    // ???????????¦¶??PSC ?? 16 ¦Ë???????0~65535??
    uint16_t presc = (uint16_t)constrain(presc_float, 0.0f, 65535.0f);

    return presc;
}

/**
 * @brief ??????????ARR
 */
static uint16_t Buzzer_Calc_ARR(buzzer_t* buzzer)
{
    if (buzzer->base_info.input_info.freq <= 0) {
        return 0; // ???????????0
    }

    // ??????ARR?
	float arr_float= buzzer->config.tim_freq/
					(buzzer->base_info.input_info.freq)/
					(buzzer->base_info.tim_presc+1.f)-1.f;
	
	// ????ARR??¦¶?????????
    return (uint16_t)constrain(arr_float, 0.0f, (float)buzzer->config.max_tim_arr);
	 //return (uint16_t)arr_float;
}

/**
 * @brief ?????????ARR????CCR
 */
static uint16_t Buzzer_Calc_CCR(buzzer_t* buzzer)
{
	return (uint16_t)constrain(
        buzzer->base_info.duty * buzzer->base_info.AAR,
        0.0f, (float)buzzer->base_info.AAR
    );
}

/**
 * @brief ??????volume?????????duty
 */
static float Buzzer_Volume_to_Duty(buzzer_t* buzzer,float volume)
{
	volume=constrain(volume,buzzer->config.min_volume,buzzer->config.max_volume);
	return (buzzer->config.max_pwm_duty-buzzer->config.min_pwm_duty)/
		(buzzer->config.max_volume-buzzer->config.min_volume)*volume+buzzer->config.min_pwm_duty;
}

/**
 * @brief Buzzer??arr??ccr???
 */
void Buzzer_Work(buzzer_t* buzzer)
{
   
    float target_freq = buzzer->base_info.input_info.freq;
    float target_volume = buzzer->base_info.input_info.volume;

    if (target_freq <= 0 || target_volume <= buzzer->config.min_volume) {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, 0); // CCR=0????????
        return;
    }
    buzzer->base_info.tim_presc = Buzzer_Calc_Optimal_Presc(buzzer, target_freq);
    buzzer->base_info.AAR = Buzzer_Calc_ARR(buzzer);
    buzzer->base_info.duty = Buzzer_Volume_to_Duty(buzzer, target_volume);

    buzzer->base_info.CCR = Buzzer_Calc_CCR(buzzer);

    __HAL_TIM_PRESCALER(&htim4, buzzer->base_info.tim_presc);  // ???????????????
    __HAL_TIM_SET_AUTORELOAD(&htim4, buzzer->base_info.AAR);   // ????ARR?????????
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, buzzer->base_info.CCR); // ?????CCR??????????
    

}


