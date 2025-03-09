#include "pwm_ctrl.h"

pwm_adc_t pwm_adc;
pwm_data_t pwm_data = {.cap_state = CAP_OFF, .power_limit = 50};
// Function to initialize PWM
void PWM_Init(void){
    HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, SET); // turn on red LED init state

    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, RESET); // make sure fets off

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED); //calibrate ADCs
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc4, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc5, ADC_SINGLE_ENDED);
    // enable HRTIM outputs
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TA2); //INL1
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TB1); //INR1
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TC1); //INL2
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TE1); //INR2

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&pwm_adc.v_cap, 1); // start ADCs
    HAL_ADC_Start_DMA(&hadc2, (uint32_t*)&pwm_adc.i_cap, 1);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)&pwm_adc.i_chassis, 1);
    HAL_ADC_Start_DMA(&hadc4, (uint32_t*)&pwm_adc.v_bat, 2);
    HAL_ADC_Start_DMA(&hadc5, (uint32_t*)&pwm_adc.i_bat, 1);

    HAL_UART_Transmit_DMA(&huart4, (uint8_t*)&pwm_data, sizeof(pwm_data)); // send UART message

    /*!!!!!!!!!!!!!!! TO BE SET!!!!!!!!!!!!!!!!!*/
    PWM_SetDutyCycle(0); // set duty cycle to 0
    PWM_SetPhase(0); // set phase to 0

    HAL_Delay(10);

    LL_HRTIM_EnableIT_REP(HRTIM1, LL_HRTIM_TIMER_MASTER);
    pwm_data.cap_state = CAP_READY;
}

__STATIC_INLINE int checkCompVal(int val){
    if(val<PWM_COMPARE_MINVAL){
        return PWM_COMPARE_MINVAL;
    }else if(val>PWM_PERIOD-100){
        return PWM_PERIOD-100;
    }else{
        return val;
    }
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_SetPhase(uint8_t phase){
    int phaseA = PWM_COMPARE_MINVAL;
    int phaseB = PWM_COMPARE_MINVAL;
    int phaseC = checkCompVal(PWM_COMPARE_MINVAL + toCompareVal(phase));
    int phaseE = checkCompVal(PWM_COMPARE_MINVAL + toCompareVal(phase));
    LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseA);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseB);
    LL_HRTIM_TIM_SetCompare3(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseC);
    LL_HRTIM_TIM_SetCompare4(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseE);
}
/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_SetDutyCycle(uint8_t dutyCycle){
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_A, dutyCycle);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_B, dutyCycle);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_C, dutyCycle);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_E, dutyCycle);
}