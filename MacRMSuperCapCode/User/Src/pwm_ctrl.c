#include "pwm_ctrl.h"

pwm_data_t pwm_data = {.cap_state = CAP_OFF, .power_limit = 50.0f};
pwm_adc_t pwm_adc;
uint32_t ready_time = 0;
uint32_t powerup_time = 0;
uint8_t master_counter = 0;

float target_cap_current;

PID_t pid={
    .p=0.0f,
    .integ=30000.0f,
    .d=0.0f,
    .i_max=0.0035f,
    .err_i=0.0f,
};

// Function to initialize PWM
void PWM_Init(void)
{
    HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, SET); // turn on red LED init state

    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, RESET); // make sure fets off

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED); // calibrate ADCs
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc4, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc5, ADC_SINGLE_ENDED);
    // enable HRTIM outputs
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TA2); // INL1
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TB1); // INR1
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TC1); // INL2
    LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TE1); // INR2

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&pwm_adc.v_cap, 1); // start ADCs
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&pwm_adc.i_cap, 1);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t *)&pwm_adc.i_chassis, 1);
    HAL_ADC_Start_DMA(&hadc4, (uint32_t *)&pwm_adc.v_bat, 2);
    HAL_ADC_Start_DMA(&hadc5, (uint32_t *)&pwm_adc.i_bat, 1);

    HAL_UART_Transmit_DMA(&huart4, (uint8_t *)&pwm_data, sizeof(pwm_data)); // send UART message

    /*!!!!!!!!!!!!!!! TO BE SET!!!!!!!!!!!!!!!!!*/
    PWM_SetDutyCycle(0); // set duty cycle to 0
    PWM_SetPhase(0);     // set phase to 0

    HAL_Delay(10);

    LL_HRTIM_EnableIT_REP(HRTIM1, LL_HRTIM_TIMER_MASTER);
    pwm_data.cap_state = CAP_READY;
    ready_time = HAL_GetTick();
}

__STATIC_INLINE int checkCompVal(int val)
{
    if (val < PWM_COMPARE_MINVAL)
    {
        return PWM_COMPARE_MINVAL;
    }
    else if (val > PWM_PERIOD - 100)
    {
        return PWM_PERIOD - 100;
    }
    else
    {
        return val;
    }
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_SetPhase(uint8_t phase)
{
    // currently has inl1 and inr1 in phase, inl2 and inr2 + phase
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
void PWM_SetDutyCycle(uint8_t dutyCycle)
{
    uint8_t leftduty, rightduty;
    // if duty cycle is less than 50%, left side is duty cycle, right side is 100%
    // if duty cycle is more than 50%, left side is 100%, right side is 100%-duty cycle
    if (dutyCycle < 127)
    {
        leftduty = dutyCycle << 1; // multiply by 2
        rightduty = 255;
    }
    else
    {
        leftduty = 255;
        rightduty = (255 - dutyCycle) << 1; // multiply by 2
    }
    int comp_left = checkCompVal(toCompareVal(leftduty));
    int comp_right = checkCompVal(toCompareVal(rightduty));
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_A, comp_left);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_B, comp_right);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_C, comp_left);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_E, comp_right);
}

__STATIC_INLINE void pid_reset(){
    //Reset the integral term
    pid.err_i= 0;
}

__STATIC_INLINE void update_pid(){

    float err = target_cap_current-pwm_data.i_cap;
    pid.err_i += err*DT;

    if(pid.err_i > pid.i_max) pid.err_i = pid.i_max;
    if(pid.err_i < -pid.i_max) pid.err_i = -pid.i_max;
    
    PWM_SetDutyCycle(pid.err_i*pid.integ);
}

__STATIC_INLINE void pid_reset_to_voltage(){
    if(pwm_data.v_bat <= pwm_data.v_cap){
        pid.err_i=(50.0f/pid.integ);  //prevent an agressive capacitor discharge
    }else{
        float io_ratio=pwm_data.v_cap/(pwm_data.v_bat+0.01f); //NEVER divide by zero
        pid.err_i=100.0f*(io_ratio/pid.integ); //if v_bat >> v_cap, io_ratio will be small
    }
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
static void fsm(void)
{
    switch (pwm_data.cap_state)
    {
    case CAP_OFF:
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, SET);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, RESET);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, RESET);
        break;
    case CAP_READY:
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, SET);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, SET);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, RESET);
        if (ready_time == 0)
        {
            ready_time = HAL_GetTick();
        }
        else if (HAL_GetTick() - ready_time < ONTIME_FILTERSTABLE_DELAY)
        {
            break;
        }
        else
        {
            ready_time = 0;
            pid_reset_to_voltage();
            pwm_data.cap_state = CAP_ON;
            powerup_time = HAL_GetTick();
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
        pwm_data.cap_state = CAP_OFF;
    }
}

void PWM_UpdateLimits(uint16_t limit)
{
    if (limit < POWER_LIMIT_MINIMUM)
    {
        limit = POWER_LIMIT_MINIMUM;
    }
    else if (limit > POWER_LIMIT_MAXIMUM)
    {
        limit = POWER_LIMIT_MAXIMUM;
    }
    pwm_data.power_limit = limit;
}

/*ADC data range is 0 to 4095
    actual voltage = (ADC data * 3.3)/4095*11
    actual current = (((ADC data * 3.3)/4095)-1.65)*20
*/
/*!!!!!!!!!!!!!!! IIR FILTER USED!!!!!!!!!!!!!!!!!*/
__STATIC_INLINE void adc_to_voltage_current(void)
{
    pwm_data.v_cap = (pwm_adc.v_cap * V_REF) / 4095.0f * 11.0f * IIR_V + pwm_data.v_cap * (1 - IIR_V);
    pwm_data.i_cap = (((pwm_adc.i_cap * V_REF) / 4095.0f) - 1.65f) * 20.0f * I_CAP_COE * IIR_C + pwm_data.i_cap * (1 - IIR_C);
    pwm_data.i_chassis = (((pwm_adc.i_chassis * V_REF) / 4095.0f) - 1.65f) * 20.0f * I_CHASSIS_COE * IIR_C + pwm_data.i_chassis * (1 - IIR_C);
    pwm_data.v_bat = (pwm_adc.v_bat * V_REF) / 4095.0f * 11.0f * IIR_V + pwm_data.v_bat * (1 - IIR_V);
    pwm_data.v_chassis = (pwm_adc.v_chassis * V_REF) / 4095.0f * 11.0f * IIR_V + pwm_data.v_chassis * (1 - IIR_V);
    pwm_data.i_bat = (((pwm_adc.i_bat * V_REF) / 4095.0f) - 1.65f) * 20.0f * I_BAT_COE * IIR_C + pwm_data.i_bat * (1 - IIR_C);
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_Control(void)
{
    adc_to_voltage_current();
    /*!!!!!!!!!! TO BE SET!!!!!!!!!!!!!!!*/
    if (master_counter == 1) 
    { // limit control changes to every other cycle
        master_counter = 0;

        target_cap_current = (pwm_data.i_bat < 0.01f || pwm_data.v_bat < 0.01f) ? pwm_data.i_chassis : ((pwm_data.power_limit / pwm_data.v_bat) - pwm_data.i_chassis);
        // calculate target current

        // FIXME: need math calculation
        float lim_judge = (pwm_data.i_bat < 0.01f || pwm_data.v_bat < 0.01f) ? CAP_MAX_CURRENT : (CAP_MAX_CURRENT * (pwm_data.v_cap / pwm_data.v_bat));
        float lim_capfull = (BAT_FULL_VOL - pwm_data.v_cap) * 7.0f;
        float lim_caplow = (pwm_data.v_cap - BAT_UVP_STARTUP_THRE) * 5.0f;

        float charge_maxi = lim_judge < lim_capfull ? lim_judge : lim_capfull;
        float discharge_maxi = lim_judge < lim_caplow ? lim_judge : lim_caplow;

        discharge_maxi = discharge_maxi < -0.25f ? -0.25f : discharge_maxi;

        target_cap_current = (target_cap_current > charge_maxi) ? charge_maxi : 
                             (target_cap_current < -discharge_maxi) ? -discharge_maxi : target_cap_current;

        HAL_IWDG_Refresh(&hiwdg);
        fsm();
    }
    else
    {
        master_counter++;
    }
}
