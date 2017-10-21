
#ifndef VV_RADIO_H
#define VV_RADIO_H

#include <bc_common.h>
#include <bcl.h>

#define VV_RADIO_THERMOSTAT 0xf0

#define VV_RADIO_DATA_TYPE_L1_POWER 0
#define VV_RADIO_DATA_TYPE_TEMPERATURE_LIVING_ROOM 1
#define VV_RADIO_DATA_TYPE_TEMPERATURE_TERRACE 2
#define VV_RADIO_DATA_TYPE_CO2 3
#define VV_RADIO_DATA_TYPE_THERMOSTAT_REFERENCE_VALUE 4

// [BUFFER_TYPE NODE_ADDRESS DATA_TYPE NEW_VALUE]
#define VV_RADIO_TYPE             (0)
#define VV_RADIO_ADDRESS          (VV_RADIO_TYPE + sizeof(uint8_t))
#define VV_RADIO_DATA_TYPE_INDEX  (VV_RADIO_ADDRESS + sizeof(uint64_t))
#define VV_RADIO_NEW_VAL          (VV_RADIO_DATA_TYPE_INDEX + sizeof(uint8_t))
#define VV_RADIO_MESSAGE_SIZE     (VV_RADIO_NEW_VAL + sizeof(float))

void vv_radio_parse_incoming_buffer(size_t *length, uint8_t *buffer);
void vv_radio_send_update(uint64_t *device_address, uint8_t data_type_index, float *new_val);

#endif
