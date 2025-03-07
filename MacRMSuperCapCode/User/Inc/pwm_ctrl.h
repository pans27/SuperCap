#ifndef PWM_CTRL_H
#define PWM_CTRL_H

#include "adc.h"
#include "gpio.h"
#include "iwdg.h"
#include "hrtim.h"
#include <stdint.h>

// Define PWM frequency and resolution
#define PWM_PERIOD 27200 // 1 kHz

// Function prototypes
void PWM_Init(void);
void PWM_SetPhase(uint8_t phase);
void PWM_SetDutyCycle(uint8_t dutyCycle);
void PWM_Control(void);

#endif // PWM_CTRL_H