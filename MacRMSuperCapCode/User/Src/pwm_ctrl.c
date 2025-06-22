#include "pwm_ctrl.h"

volatile uint8_t uart_tx_done = 1;
pwm_data_t pwm_data = {.cap_state = CAP_OFF, .power_limit = 55.0f};
pwm_adc_t pwm_adc;
uint32_t ready_time = 0;
uint32_t powerup_time = 0;
uint32_t protection_triggered=0;
uint16_t master_counter = 0;
uint8_t pwm_msg[] = "hello world\n";
uint8_t duty, leftduty, rightduty;

float target_current;

PID_t pid={
    .p=0.07f, 
    .integ=60000.0f,
    .d=0.0f,
    .i_max=0.00175f,
    .err_i=0.0f,
};

#define DISABLE_FETS 0 // set to 1 to disable FETs

static void cap_fsm(void);
// Function to initialize PWM
void PWM_Init(void)
{
    HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, LED_ON); // turn on red LED init state

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

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&(pwm_adc.v_cap), 1); // start ADCs
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)&(pwm_adc.i_cap), 1);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t *)&(pwm_adc.i_chassis), 1);
    HAL_ADC_Start_DMA(&hadc4, (uint32_t *)&(pwm_adc.v_bat), 2);
    HAL_ADC_Start_DMA(&hadc5, (uint32_t *)&(pwm_adc.i_bat), 1);

    
    //HAL_UART_Transmit_DMA(&huart4, pwm_msg, sizeof(pwm_msg)); // send UART message
    toUart("hello world\r\n");
    //HAL_UART_Transmit_DMA(&huart4, (uint8_t *)&pwm_data, sizeof(pwm_data)); // send UART message
    //HAL_UART_Transmit_DMA(&huart4, pwm_msg, sizeof(pwm_msg)); // send UART message
    //toUart("hello world\n");

    /*!!!!!!!!!!!!!!! TO BE SET!!!!!!!!!!!!!!!!!*/
    PWM_SetDutyCycle(0.0f); // set duty cycle to 0
    PWM_SetPhase(13);     // set phase to 0

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

void toUart( char *ptr)
{
    while (!uart_tx_done); // wait for previous TX to complete
    uart_tx_done = 0;

    HAL_UART_Transmit_DMA(&huart4, (uint8_t *)ptr, strlen(ptr));
    //return len;
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == UART4)  // match your UART
    {
        uart_tx_done = 1;
    }
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_SetPhase(uint8_t phase)
{
    // currently has inl1 and inr1 in phase, inl2 and inr2 + phase
    int phaseA = PWM_COMPARE_MINVAL;
    int phaseB = checkCompVal(PWM_COMPARE_MINVAL + toCompareVal(phase));
    int phaseC = checkCompVal(PWM_COMPARE_MINVAL + PWM_PERIOD / 2);
    int phaseE = checkCompVal(PWM_COMPARE_MINVAL + PWM_PERIOD / 2 + toCompareVal(phase));
    LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseA);
    LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseB);
    LL_HRTIM_TIM_SetCompare3(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseC);
    LL_HRTIM_TIM_SetCompare4(HRTIM1, LL_HRTIM_TIMER_MASTER, phaseE);
}
/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_SetDutyCycle(float dutyCycle)
{   
    if(dutyCycle<0.1f){
        dutyCycle=0.1f;
    }else if(dutyCycle>100.0f){
        dutyCycle=100.0f;
    }

    duty = (uint8_t)(dutyCycle*255.0f/100.0f);
    // if duty cycle is less than 50%, left side is duty cycle, right side is 100%
    // if duty cycle is more than 50%, left side is 100%, right side is 100%-duty cycle
    if (duty <= 127)
    {
        leftduty = duty << 1; // multiply by 2
        rightduty = 255;
    }
    else
    {
        leftduty = 255;
        rightduty = (255 - duty) << 1; // multiply by 2
    }
    // if(master_counter==1000){
    //     char line[100],line2[100],line3[100];
    //     sprintf(line, "bat voltage: %2.3f, cap voltage: %2.3f\r\n", pwm_data.v_bat, pwm_data.v_cap);
    //     toUart(line);
    //     sprintf(line2,"left duty: %d\r\n", leftduty);
    //     toUart(line2);
    //     sprintf(line3,"right duty: %d\r\n", rightduty);
    //     toUart(line3);
    //     master_counter = 0;
    // }

    int comp_left = checkCompVal(toCompareVal(leftduty));
    int comp_right = checkCompVal(toCompareVal(rightduty));
    LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_A, comp_left);
    LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_B, comp_right);
    LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_C, comp_left);
    LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_E, comp_right);
}

__STATIC_INLINE void pid_reset(){
    //Reset the integral term
    pid.err_i= 0;
}

__STATIC_INLINE void update_pid(){

    float err = target_current-pwm_data.i_bat;
    pid.err_i += err*DT;

    if(pid.err_i > pid.i_max) pid.err_i = pid.i_max;
    if(pid.err_i < -pid.i_max) pid.err_i = -pid.i_max;
    
    PWM_SetDutyCycle(pid.err_i*pid.integ+ pid.p*err);
}

__STATIC_INLINE void pid_reset_to_voltage(){
    if(pwm_data.v_bat <= pwm_data.v_cap){
        pid.err_i=(50.0f/pid.integ);  //prevent an agressive capacitor discharge
    }else{
        float io_ratio=pwm_data.v_cap/(2.0f*(pwm_data.v_bat+0.01f)); //NEVER divide by zero
        pid.err_i=100.0f*(io_ratio/pid.integ); //if v_bat >> v_cap, io_ratio will be small
    }
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
static void cap_fsm(void)
{
    if(pwm_data.cap_state != CAP_READY){
        if(pwm_data.v_chassis > BUS_OVP_THRE){ //BUS over-voltage protection
            pwm_data.cap_state=VBUS_OVP;
            protection_triggered=HAL_GetTick();
        }else if(pwm_data.v_chassis < BUS_UVP_THRE){ //BUS under-voltage halt
            pwm_data.cap_state=VBUS_UVP;
            protection_triggered=HAL_GetTick();
        }else if(pwm_data.v_cap > BAT_OVP_THRE){ //BAT over-voltage protection
            pwm_data.cap_state=VBAT_OVP;
            //data.testval=data.v_cap;// debug display output
            protection_triggered=HAL_GetTick();
        }else if(pwm_data.cap_state==VBUS_UVP && pwm_data.v_chassis > BUS_UVP_THRE \
            && protection_triggered <( HAL_GetTick() - 50)){ 
            //recovery from BUS UVP
            pid_reset();
            protection_triggered=HAL_GetTick();
            pwm_data.cap_state=CAP_READY;
        }else if(pwm_data.cap_state==VBUS_OVP && pwm_data.v_chassis < BUS_OVP_THRE \
            && protection_triggered <( HAL_GetTick() - 50)){  
            //recovery from BUS OVP
            pid_reset();
            protection_triggered=HAL_GetTick();
            pwm_data.cap_state=CAP_READY;
        }
    }

    switch (pwm_data.cap_state)
    {
    case CAP_OFF:
#if (DISABLE_FETS)
        
            HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET); // turn off fets
        
#endif
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, LED_ON);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, LED_OFF);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, LED_OFF);
        break;
    case CAP_READY:
#if (DISABLE_FETS)
        
            HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET); // turn off fets
        
#endif
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, LED_ON);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, LED_ON);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, LED_OFF);
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
#if (!DISABLE_FETS)
            HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET); // turn on fets
#endif
        }
        break;
    case CAP_ON:
#if (DISABLE_FETS)
        
            HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET); // turn off fets
        
#endif
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, LED_OFF);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, LED_ON);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, LED_OFF);
        update_pid();
        break;
    case VBUS_OVP:
        HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);// turn off fets
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, LED_OFF);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, LED_OFF);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, LED_ON);
        break;
    case VBUS_UVP:
        HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, RESET);// turn off fets
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, LED_ON);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, LED_OFF);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, LED_ON); 
        break;
    case VBAT_OVP:
#if (DISABLE_FETS)
        
            HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET); // turn off fets
        
#endif
        if(HAL_GetTick() > protection_triggered + PROTECTION_RECOVERY_TIME){
            pid_reset();
            pwm_data.cap_state=CAP_READY;
        }
#if (DISABLE_FETS)
        
            HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET); // turn off fets
        
#endif
        HAL_GPIO_WritePin(R_GPIO_Port, R_Pin, LED_OFF);
        HAL_GPIO_WritePin(G_GPIO_Port, G_Pin, LED_ON);
        HAL_GPIO_WritePin(B_GPIO_Port, B_Pin, LED_ON);
        break;
    default:
        pwm_data.cap_state = CAP_OFF;
    }
}

void PWM_UpdateLimits(float limit)
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
    pwm_data.i_cap = (((pwm_adc.i_cap * V_REF) / 4095.0f) - V_REF/2.0f) * 20.0f * IIR_C + pwm_data.i_cap * (1 - IIR_C);
    pwm_data.i_chassis = ((((pwm_adc.i_chassis * V_REF) / 4095.0f) - V_REF/2.0f) * 20.0f + 0.1f)* IIR_C + pwm_data.i_chassis * (1 - IIR_C);
    pwm_data.v_bat = (pwm_adc.v_bat * V_REF) / 4095.0f * 11.0f * IIR_V + pwm_data.v_bat * (1 - IIR_V);
    pwm_data.v_chassis = (pwm_adc.v_chassis * V_REF) / 4095.0f * 11.0f * IIR_V + pwm_data.v_chassis * (1 - IIR_V);
    pwm_data.i_bat = ((((pwm_adc.i_bat * V_REF) / 4095.0f) - V_REF/2.0f) * 20.0f + 0.1f)* IIR_C + pwm_data.i_bat * (1 - IIR_C);
}

void send_uart(void)
{
    char line1[100], line2[100], line3[100], line4[100], line5[100], line6[100], line7[100], line8[100];
    sprintf(line1, "v_cap: %2.3f\r\n", pwm_data.v_cap);
    toUart(line1);
    sprintf(line2, "i_cap: %2.3f\r\n", pwm_data.i_cap);
    toUart(line2);
    sprintf(line3, "v_chassis: %2.3f\r\n", pwm_data.v_chassis);
    toUart(line3);
    sprintf(line4, "i_chassis: %2.3f\r\n", pwm_data.i_chassis);
    toUart(line4);
    sprintf(line5, "v_bat: %2.3f\r\n", pwm_data.v_bat);
    //sprintf(line5, "v_bat: %d\r\n", pwm_adc.v_bat);
    toUart(line5);
    sprintf(line6, "i_bat: %2.3f\r\n", pwm_data.i_bat);
    toUart(line6);
    sprintf(line7, "power_limit: %2.3f\r\n", pwm_data.power_limit);
    toUart(line7);
    sprintf(line8, "cap_state: %d\r\n", pwm_data.cap_state);
    toUart(line8);
}

/*!!!!!!!!!!!!!!! ALGORITHM NEEDED!!!!!!!!!!!!!!!!!*/
void PWM_Control(void)
{
    adc_to_voltage_current();
    /*!!!!!!!!!! TO BE SET!!!!!!!!!!!!!!!*/
    // if (master_counter == 1) 
    // { // limit control changes to every other cycle
        //master_counter = 0;
        //abs(pwm_data.i_bat) < 0.1f || ( abs(pwm_data.v_bat) < 0.1f) ? -pwm_data.i_chassis :
        target_current =  ((pwm_data.power_limit / pwm_data.v_bat) - pwm_data.i_chassis);
        // calculate target current

        // FIXME: need math calculation
        //abs(pwm_data.i_bat) < 0.1f || ??
        float lim_judge = (abs(pwm_data.v_bat) < 0.1f ) ? CAP_MAX_CURRENT : (CAP_MAX_CURRENT * (pwm_data.v_cap / pwm_data.v_bat));
        float lim_capfull = (BAT_FULL_VOL - pwm_data.v_cap) * 7.0f;
        float lim_caplow = (pwm_data.v_cap - BAT_UVP_STARTUP_THRE) * 5.0f;

        float charge_maxi = lim_judge < lim_capfull ? lim_judge : lim_capfull;
        float discharge_maxi = lim_judge < lim_caplow ? lim_judge : lim_caplow;

        discharge_maxi = discharge_maxi < -0.25f ? -0.25f : discharge_maxi;

        target_current = (target_current > charge_maxi) ? charge_maxi : 
                             (target_current < -discharge_maxi) ? -discharge_maxi : target_current;
        target_current = target_current + pwm_data.i_chassis;
        HAL_IWDG_Refresh(&hiwdg);
        cap_fsm();
    // }
    // else
    // {
    master_counter++;
    // }
}
