
#include "vv_radio.h"

#include <bc_common.h>
#include <bcl.h>

void vv_radio_send_update(uint64_t *device_address, uint8_t data_type_index, float *new_val) {
    static uint8_t buffer[VV_RADIO_MESSAGE_SIZE];    
    buffer[VV_RADIO_TYPE] = VV_RADIO_THERMOSTAT;
    memcpy(buffer + VV_RADIO_ADDRESS, device_address, sizeof(uint64_t));    
    buffer[VV_RADIO_DATA_TYPE_INDEX] = data_type_index;
    memcpy(buffer + VV_RADIO_NEW_VAL, new_val, sizeof(*new_val));    

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

void json_radio_send(uint64_t *device_address, const char *json) {
    static uint8_t buffer[MAX_JSON_SIZE];
    memset(buffer, 0, MAX_JSON_SIZE);

    size_t json_length = strlen(json);

    buffer[JSON_RADIO_TYPE_POS] = JSON_RADIO_TYPE;
    memcpy(buffer + JSON_RADIO_ADDRESS_POS, device_address, sizeof(uint64_t));    
    memcpy(buffer + JSON_RADIO_SIZE_POS, &json_length, sizeof(size_t));        
    memcpy(buffer + JSON_RADIO_VALUE_POS, json, strlen(json) * sizeof(char));        

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}
