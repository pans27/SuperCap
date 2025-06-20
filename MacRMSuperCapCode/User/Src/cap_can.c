#include "cap_can.h"

FDCAN_FilterTypeDef sFilterConfig1;
FDCAN_FilterTypeDef sFilterConfig2;

capcan_rx_t rx_data;
capcan_tx_t tx_data;

// Default Tx Header configuration
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

// Array to map DLC values to data length in bytes
uint8_t dlc2len[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

extern pwm_data_t pwm_data;

// FDCAN Rx FIFO 0 callback function
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if (RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE)
    {
        FDCAN_RxHeaderTypeDef rx_header;

        HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, (uint8_t *)&rx_data);

        if(*(&rx_header.Identifier) == CAN_RX_ID)
        {
            // Update PWM limits based on received data
            PWM_UpdateLimits((float)rx_data.power_limit/100.0f); //scale down the power limit by 100
        }
    }
}

// FDCAN2 Configuration
void fdcan2_config(void)
{
  // Configure filter for standard IDs
  sFilterConfig1.IdType = FDCAN_STANDARD_ID;
  sFilterConfig1.FilterIndex = 0;
  sFilterConfig1.FilterType = FDCAN_FILTER_RANGE;
  sFilterConfig1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig1.FilterID1 = 0x00;
  sFilterConfig1.FilterID2 = 0x7FF;
  if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig1) != HAL_OK)
  {
    Error_Handler(); // Handle filter config error
  }

  // Configure filter for extended IDs
  sFilterConfig2.IdType = FDCAN_EXTENDED_ID;
  sFilterConfig2.FilterIndex = 0;
  sFilterConfig2.FilterType = FDCAN_FILTER_RANGE;
  sFilterConfig2.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  sFilterConfig2.FilterID1 = 0x00;
  sFilterConfig2.FilterID2 = 0x1FFFFFFF;
  if (HAL_FDCAN_ConfigFilter(&hfdcan2, &sFilterConfig2) != HAL_OK)
  {
    Error_Handler(); // Handle filter config error
  }

  // Configure global filter to reject remote frames
  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
  {
    Error_Handler(); // Handle global filter config error
  }

  // Activate Rx FIFO 0 new message notification
  if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
  {
    Error_Handler(); // Handle FIFO 0 notification activation error
  }

  // Activate Bus Off notification
  if (HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_BUS_OFF, 0) != HAL_OK)
  {
    Error_Handler(); // Handle Bus Off notification activation error
  }

  // Configure and enable Tx Delay Compensation for BRS mode
  HAL_FDCAN_ConfigTxDelayCompensation(&hfdcan2, hfdcan2.Init.DataPrescaler * hfdcan2.Init.DataTimeSeg1, 0);
  HAL_FDCAN_EnableTxDelayCompensation(&hfdcan2);

  HAL_FDCAN_Start(&hfdcan2); // Start FDCAN module
  HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO0_NEW_MESSAGE,0); // Activate RX FIFO0 new message interrupt
}



// Instances of the FDCAN_SendFailTypeDef structure for each FDCAN instance
FDCAN_SendFailTypeDef fdcan1_send_fail = {0};
FDCAN_SendFailTypeDef fdcan2_send_fail = {0};
FDCAN_SendFailTypeDef fdcan3_send_fail = {0};

// Function to get data length from DLC value
uint8_t can_dlc2len(uint32_t RxHeader_DataLength)
{
  return dlc2len[RxHeader_DataLength>>16]; // Return data length based on DLC value
}

// Function to transmit CAN messages using FDCAN2
void fdcan2_transmit(uint32_t can_id, uint32_t DataLength, uint8_t tx_data[])
{
  TxHeader2.Identifier = can_id; // Set CAN identifier
  TxHeader2.IdType = FDCAN_EXTENDED_ID; // Set ID type to extended

  // If CAN ID is less than 0x800, use standard ID
  if(can_id < 0x800) {
    TxHeader2.IdType = FDCAN_STANDARD_ID;
  }

  // Add message to Tx FIFO
  if(HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader2, tx_data) != HAL_OK) {
    fdcan2_send_fail.flag = 1; // Set flag to indicate send failure
    memcpy(&fdcan2_send_fail.TxHeader, &TxHeader2, sizeof(FDCAN_TxHeaderTypeDef)); // Copy Tx header to send fail structure
    memcpy(fdcan2_send_fail.TxData, tx_data, can_dlc2len(DataLength)); // Copy Tx data to send fail structure
  }
}

// Function to transmit cap data
void cap_transmit(void)
{
    tx_data.current_chassis_power = (uint16_t)(pwm_data.i_chassis * pwm_data.v_chassis);
    tx_data.current_battery_power = (uint16_t)(pwm_data.i_bat * pwm_data.v_bat);
    tx_data.cap_voltage = (uint16_t)pwm_data.v_cap;
    tx_data.cap_state = (uint16_t)pwm_data.cap_state;
    fdcan2_transmit(CAN_TX_ID, FDCAN_DLC_BYTES_8, (uint8_t*)&tx_data);
}

