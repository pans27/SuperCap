#ifndef PWM_CTRL_H
#define PWM_CTRL_H

#include "adc.h"
#include "gpio.h"
#include "iwdg.h"
#include "hrtim.h"
#include "usart.h"
#include <stdint.h>
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
// Define PWM frequency and resolution
#define PWM_PERIOD 13600 // 400 kHz
#define PWM_COMPARE_MINVAL (700)
//expect p = uint8_t values, 0 to 255
#define toCompareVal(p) (p*PWM_PERIOD)/255

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

#define ONTIME_FILTERSTABLE_DELAY 10
/*************************************/

#define IIR_V 0.1f
#define IIR_C 0.1f
#define I_CAP_COE 0.72f
#define I_CHASSIS_COE 0.63f
#define I_BAT_COE 0.65f
#define V_REF 3.23f

#define DT (1.0f/400000.0f) // 5 x PWM period

typedef struct pwm_adc_t{
    uint16_t v_cap;   //ADC1_IN2 (PA1)
    uint16_t i_cap;     //ADC2_IN1 (PA0)
    uint16_t i_chassis;    //ADC3_IN1(PB1)
    uint16_t v_bat;     //ADC4_IN4(PB14)
    uint16_t v_chassis;    //ADC4_IN5(PB15)
    uint16_t i_bat;     //ADC5_IN1(PA8)
}pwm_adc_t;

typedef struct pwm_data_t
{
    float v_cap;
    float i_cap;
    float v_chassis;     
    float i_chassis; 
    float v_bat;
    float i_bat; 
    float power_limit;
    uint8_t cap_state;
}pwm_data_t;

typedef struct pid_t
{
    const float p;
    const float integ;
    const float d;
    const float i_max;
    float err_i;
}PID_t;

enum cap_states{
    CAP_OFF,
    CAP_READY,
    CAP_ON,
    VBUS_OVP,
    VBUS_UVP,
    VBAT_OVP,
};

enum led_states{
    LED_ON,
    LED_OFF
};


// Function prototypes
void PWM_Init(void);
void PWM_SetPhase(uint8_t phase);
void PWM_SetDutyCycle(float dutyCycle);
void PWM_Control(void);
void PWM_UpdateLimits(float limit);
void toUart( char *ptr);
void send_uart(void);
static void cap_fsm(void);

#endif // PWM_CTRL_H
