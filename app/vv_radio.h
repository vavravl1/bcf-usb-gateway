
#ifndef VV_RADIO_H
#define VV_RADIO_H

#include <bc_common.h>
#include <bcl.h>
#include <jsmn.h>


#define VV_RADIO_THERMOSTAT 0xf0

#define VV_RADIO_DATA_TYPE_L1_POWER 0
#define VV_RADIO_DATA_TYPE_FVE_POWER 1
#define VV_RADIO_DATA_TYPE_TEMPERATURE_LIVING_ROOM 2
#define VV_RADIO_DATA_TYPE_TEMPERATURE_TERRACE 3
#define VV_RADIO_DATA_TYPE_TEMPERATURE_BEDROOM 4
#define VV_RADIO_DATA_TYPE_CO2 5
#define VV_RADIO_DATA_TYPE_THERMOSTAT_REFERENCE_VALUE 6

// [BUFFER_TYPE NODE_ADDRESS DATA_TYPE NEW_VALUE]
#define VV_RADIO_TYPE             (0)
#define VV_RADIO_ADDRESS          (VV_RADIO_TYPE + sizeof(uint8_t))
#define VV_RADIO_DATA_TYPE_INDEX  (VV_RADIO_ADDRESS + sizeof(uint64_t))
#define VV_RADIO_NEW_VAL          (VV_RADIO_DATA_TYPE_INDEX + sizeof(uint8_t))
#define VV_RADIO_MESSAGE_SIZE     (VV_RADIO_NEW_VAL + sizeof(float))

#define JSON_RADIO_TYPE           0xf1
#define JSON_RADIO_TYPE_POS       (0)
#define JSON_RADIO_ADDRESS_POS    (JSON_RADIO_TYPE_POS + sizeof(uint8_t))
#define JSON_RADIO_SIZE_POS       (JSON_RADIO_ADDRESS_POS + sizeof(uint64_t))
#define JSON_RADIO_VALUE_POS      (JSON_RADIO_SIZE_POS + sizeof(size_t))

#define MAX_JSON_SIZE 50

void vv_radio_send_update(uint64_t *device_address, uint8_t data_type_index, float *new_val);
void json_radio_send(uint64_t *device_address, const char *json);

#endif
