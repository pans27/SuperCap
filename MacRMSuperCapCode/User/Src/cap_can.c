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

void fdcan2_config(void)
{

  sFilterConfig2.IdType = FDCAN_EXTENDED_ID;
  sFilterConfig2.FilterIndex = 0;
  sFilterConfig2.FilterType = FDCAN_FILTER_RANGE;
  sFilterConfig2.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig2.FilterID1 = 0x00;
  sFilterConfig2.FilterID2 = 0x1FFFFFFF;
  if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig2) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure global filter on both FDCAN instances:
  Filter all remote frames with STD and EXT ID
  Reject non matching frames with STD ID and EXT ID */
  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
  {
    Error_Handler();
  }

  /* Activate Rx FIFO 0 new message notification on both FDCAN instances */
  if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_BUS_OFF, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure and enable Tx Delay Compensation, required for BRS mode.
        TdcOffset default recommended value: DataTimeSeg1 * DataPrescaler
        TdcFilter default recommended value: 0 */
  HAL_FDCAN_ConfigTxDelayCompensation(&hfdcan2, hfdcan2.Init.DataPrescaler * hfdcan2.Init.DataTimeSeg1, 0);
  HAL_FDCAN_EnableTxDelayCompensation(&hfdcan2);

  HAL_FDCAN_Start(&hfdcan2);
  HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE,0);
}


void fdcan2_transmit(void)
{
    tx_data.current_chassis_power = (uint16_t)(pwm_data.i_chassis * pwm_data.v_chassis);
    tx_data.current_battery_power = (uint16_t)(pwm_data.i_bat * pwm_data.v_bat);
    tx_data.cap_voltage = (uint16_t)pwm_data.v_cap;
    tx_data.cap_state = (uint16_t)pwm_data.cap_state;
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader2, &tx_data);
}

