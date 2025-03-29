#ifndef CAP_CAN_H
#define CAP_CAN_H

#include <stdint.h>
#include "fdcan.h"
#include "pwm_ctrl.h"

// CAN communication defines
#define CAN_TX_ID    0x301   // CAN transmit ID
#define CAN_RX_ID    0x302   // CAN receive ID

typedef struct capcan_rx_t{
    uint16_t power_limit;
    uint16_t buffer;
    uint16_t rsvd;
}capcan_rx_t;

typedef struct capcan_tx_t{
    uint16_t current_chassis_power;
    uint16_t current_battery_power;
    uint16_t cap_voltage;
    uint16_t cap_state;
}capcan_tx_t;

void CAN_Init(void);
void CAN_ReceiveData(capcan_rx_t *data);
void fdcan2_transmit(void);
#endif // CAP_CAN_H