#include <ctype.h>
#include "tiger1200_g3.h"
#include "driver/twai.h"
#include "tpms.h"
#include "esp_log.h"

#ifdef CONFIG_CT_VEHICLE_TIGER1200_GEN3

    uint16_t tpms_counter = 0;

    void tiger1200_g3_tpms_send(ct_tpms_state_t* tpms_state) {

        twai_message_t tx_msg;
        ct_tiger_1200_g3_tpms_packet_t tpms_packet;

        tx_msg.identifier = 0x600;
        tx_msg.data_length_code = 8;

        TPMS_ENTER_LOCK();

        tpms_packet.counter = tpms_counter;
        tpms_packet.status = V_BATTERY_OK | V_PRESSURE_STABLE;
        tpms_packet.unused = 0;
        tpms_packet.fault = V_FAULT_OK | V_TPMS_WARNING_OFF;

        if(tpms_counter % 2 == 0) {
            tpms_packet.wheel = V_FRONT_WHEEL;
            tpms_packet.status = (tpms_state->wheel[0].battery > 25 ? V_BATTERY_OK : V_BATTERY_LOW) | V_PRESSURE_STABLE;
            tpms_packet.pressure = tpms_state->wheel[0].pressure * 5;
            tpms_packet.temp = tpms_state->wheel[0].temperature + 50;
        } else { 
            tpms_packet.wheel = V_REAR_WHEEL;
            tpms_packet.status = (tpms_state->wheel[1].battery > 25 ? V_BATTERY_OK : V_BATTERY_LOW) | V_PRESSURE_STABLE;
            tpms_packet.fault = V_FAULT_OK |V_TPMS_WARNING_OFF;
            tpms_packet.pressure = tpms_state->wheel[1].pressure * 5;
            tpms_packet.temp = tpms_state->wheel[1].temperature + 50;
        }

        TPMS_EXIT_LOCK();

        memcpy(tx_msg.data, (uint8_t*) &tpms_packet, 8);
        ESP_LOGI("TIGER", "Front: %d, Rear: %d", tpms_state->wheel[0].pressure, tpms_state->wheel[1].pressure);
        ESP_LOGI("TIGER", "0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X", tx_msg.data[0], tx_msg.data[1],tx_msg.data[2], tx_msg.data[3], tx_msg.data[4], tx_msg.data[5],tx_msg.data[6],tx_msg.data[7]);

        twai_transmit(&tx_msg, pdMS_TO_TICKS(100));
        tpms_counter++;
    }

#endif