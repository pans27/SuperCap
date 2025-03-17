#include "pwm_ctrl.h"

pwm_data_t pwm_data = {.cap_state = CAP_OFF, .power_limit = 50};

uint32_t ready_time = 0;
uint32_t powerup_time = 0;
uint8_t master_counter = 0;
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

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&pwm_data.pwm_adc.v_cap, 1); // start ADCs
    HAL_ADC_Start_DMA(&hadc2, (uint32_t*)&pwm_data.pwm_adc.i_cap, 1);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)&pwm_data.pwm_adc.i_chassis, 1);
    HAL_ADC_Start_DMA(&hadc4, (uint32_t*)&pwm_data.pwm_adc.v_bat, 2);
    HAL_ADC_Start_DMA(&hadc5, (uint32_t*)&pwm_data.pwm_adc.i_bat, 1);

    HAL_UART_Transmit_DMA(&huart4, (uint8_t*)&pwm_data, sizeof(pwm_data)); // send UART message

    /*!!!!!!!!!!!!!!! TO BE SET!!!!!!!!!!!!!!!!!*/
    PWM_SetDutyCycle(0); // set duty cycle to 0
    PWM_SetPhase(0); // set phase to 0

    HAL_Delay(10);

    LL_HRTIM_EnableIT_REP(HRTIM1, LL_HRTIM_TIMER_MASTER);
    pwm_data.cap_state = CAP_READY;
    ready_time = HAL_GetTick();
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
    //currently has inl1 and inr1 in phase, inl2 and inr2 + phase
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
    uint8_t leftduty,rightduty;
    //if duty cycle is less than 50%, left side is duty cycle, right side is 100%
    //if duty cycle is more than 50%, left side is 100%, right side is 100%-duty cycle
    if(dutyCycle<127){
        leftduty=dutyCycle<<1; //multiply by 2
        rightduty=255;
    }else{
        leftduty=255;
        rightduty=(255-dutyCycle)<<1; //multiply by 2
    }
    int comp_left = checkCompVal(toCompareVal(leftduty));
    int comp_right = checkCompVal(toCompareVal(rightduty));    
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_A, comp_left);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_B, comp_right);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_C, comp_left);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_E, comp_right);
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
static void fsm(void){
    switch(pwm_data.cap_state){
        case CAP_OFF:
            HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, SET);
            HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, RESET);
            HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, RESET);
            break;
        case CAP_READY:
            HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, SET);
            HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, SET);
            HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, RESET);
            if(ready_time==0){
                ready_time=HAL_GetTick();
            }else if(HAL_GetTick()-ready_time < ONTIME_FILTERSTABLE_DELAY){
                break;
            }else{
                ready_time=0;
                pid_reset_to_voltage();
                pwm_data.cap_state=CAP_ON;
                powerup_time=HAL_GetTick();
                HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, SET); // turn on fets
            }
            break;
        case CAP_ON:
            HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, RESET);
            HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, SET);
            HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, RESET);
            update_pid();
            break;
        case VBUS_OVP:
        case VBUS_UVP:
        case VBAT_OVP:
        default:
            pwm_data.cap_state=CAP_OFF;
    }
}

void PWM_UpdateLimits(uint8_t limit){
    if(limit<POWER_LIMIT_MINIMUM){
        limit=POWER_LIMIT_MINIMUM;
    }else if(limit>POWER_LIMIT_MAXIMUM){
        limit=POWER_LIMIT_MAXIMUM;
    }
    pwm_data.power_limit = limit;
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_control(void){
    /*!!!!!!!!!! TO BE SET!!!!!!!!!!!!!!!*/
    if(master_counter==5){ // limit control changes to every 5th cycle
        master_counter = 0;
        fsm();
    } else{
        master_counter++;
    }
}
