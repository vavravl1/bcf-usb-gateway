
#include "vv_radio.h"

#include <bc_common.h>
#include <bcl.h>

void vv_radio_send_update(struct vv_radio_single_float_packet *source) {
    static uint8_t buffer[VV_RADIO_MESSAGE_SIZE];    
    buffer[VV_RADIO_TYPE] = VV_RADIO_SINGLE_FLOAT;
    memcpy(buffer + VV_RADIO_ADDRESS, &source -> device_address, sizeof(uint64_t));    
    buffer[VV_RADIO_DATA_TYPE] = source -> type;
    memcpy(buffer + VV_RADIO_VALUE, &source -> value, sizeof(float));    

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

void vv_radio_parse_incoming_buffer(size_t *length, uint8_t *buffer, struct vv_radio_single_float_packet *target) {
    if (*length != VV_RADIO_MESSAGE_SIZE) { 
	return;
    }

    target -> type = buffer[VV_RADIO_DATA_TYPE];
    memcpy(& target -> device_address, buffer + VV_RADIO_ADDRESS, sizeof(uint64_t));
    memcpy(& target -> value, buffer + VV_RADIO_VALUE, sizeof(float));
}

