#ifndef PWM_CTRL_H
#define PWM_CTRL_H

#include "adc.h"
#include "gpio.h"
#include "iwdg.h"
#include "hrtim.h"
#include <stdint.h>

// Define PWM frequency and resolution
#define PWM_PERIOD 27200 // 1 kHz
#define toCompareVal(p) (p*DCDC_PERIOD)/100

/************SAFETY SETTINGS**********/
#define BUS_UVP_THRE 18.0f
#define BUS_OVP_THRE 28.5f
#define BAT_OVP_THRE 30.0f
#define BAT_FULL_VOL 23.5f
#define BAT_UVP_STARTUP_THRE 10.0f

#define PROTECTION_RECOVERY_TIME 2000

#define CAP_MAX_CURRENT 13.8f

#define POWER_LIMIT_MINIMUM 15.0f
#define POWER_LIMIT_MAXIMUM 350.0f

#define ONTIME_FILTERSTABLE_DELAY 5
/*************************************/

typedef struct pwr_adc_t{
    uint16_t v_cap;   //ADC1_IN2 (PA1)
    uint16_t i_cap;     //ADC2_IN1 (PA0)
    uint16_t i_chassis;    //ADC3_IN1(PB1)
    uint16_t v_bat;     //ADC4_IN4(PB14)
    uint16_t v_chassis;    //ADC4_IN5(PB15)
    uint16_t i_bat;     //ADC5_IN1(PA8)
}pwr_adc_t;

// Function prototypes
void PWM_Init(void);
void PWM_SetPhase(uint8_t phase);
void PWM_SetDutyCycle(uint8_t dutyCycle);
void PWM_Control(void);
void PWM_UpdateLimits(uint8_t limit);

#endif // PWM_CTRL_H