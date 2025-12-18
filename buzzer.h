#ifndef __BUZZER_H
#define __BUZZER_H

#include "tim.h"
/* Private function prototypes -----------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
typedef struct buzzer_config_struct {
	uint32_t max_tim_arr;
	uint32_t tim_freq;
	
	float min_pwm_duty;
	float max_pwm_duty;
	float	 min_volume;
	float 	 max_volume;
}buzzer_config_t;


// ????-????-??????‰Ø?????????????????????
typedef struct note_duration {
	float volume;				/* ???? */
    float freq;     			/* ????????Hz?? */
    uint32_t duration; 			/* ????????????? */
}note_duration_t;

/* ?????? */
typedef struct buzzer_input_info_struct {
	float volume;				 
    float freq;   		
}buzzer_input_info_t;

typedef struct buzzer_status_struct {
	buzzer_input_info_t input_info;
	uint16_t tim_presc;
	float duty;
  	uint16_t CCR;
	uint16_t AAR;
}buzzer_base_info_t;

typedef struct buzzer_struct
{
	buzzer_config_t config;
	buzzer_base_info_t base_info;

	void (*init)(struct buzzer_struct* buz_str);
	void (*work)(struct buzzer_struct* buz_str);
} buzzer_t;


/* Exported function --------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern buzzer_t buzzer;


#endif
