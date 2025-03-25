#include "cap_can.h"

capcan_rx_t rx_data;
FDCAN_FilterTypeDef sFilterConfig2;
capcan_tx_t tx_data;
FDCAN_TxHeaderTypeDef TxHeader2 = {
    .Identifier = CAN_TX_ID,
    .IdType = FDCAN_EXTENDED_ID,
    .TxFrameType = FDCAN_DATA_FRAME,
    .DataLength = FDCAN_DLC_BYTES_8,
    .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
    .BitRateSwitch = FDCAN_BRS_ON,
    .FDFormat = FDCAN_CLASSIC_CAN,
    .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
    .MessageMarker = 0
};

extern pwm_data_t pwm_data;

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)
    {
        FDCAN_RxHeaderTypeDef rx_header;

        HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, (uint8_t *)&rx_data);

        if(*(&rx_header.Identifier) == CAN_RX_ID)
        {
            // Use them as needed
            PWM_UpdateLimits((float)rx_data.power_limit);
        }
    }
}

void fdcan2_transmit(void)
{
    tx_data.current_chassis_power = (uint16_t)(pwm_data.i_chassis * pwm_data.v_chassis);
    tx_data.current_battery_power = (uint16_t)(pwm_data.i_bat * pwm_data.v_bat);
    tx_data.cap_voltage = (uint16_t)pwm_data.v_cap;
    tx_data.cap_state = (uint16_t)pwm_data.cap_state;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader2, &tx_data);
}

