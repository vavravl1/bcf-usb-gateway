
#ifndef VV_RADIO_H
#define VV_RADIO_H

#include <bc_common.h>
#include <bcl.h>
#include <jsmn.h>


#define VV_RADIO_SINGLE_FLOAT 0xf0

#define VV_RADIO_DATA_TYPE_L1_POWER 0
#define VV_RADIO_DATA_TYPE_FVE_POWER 1
#define VV_RADIO_DATA_TYPE_TEMPERATURE_LIVING_ROOM 2
#define VV_RADIO_DATA_TYPE_TEMPERATURE_TERRACE 3
#define VV_RADIO_DATA_TYPE_TEMPERATURE_BEDROOM 4
#define VV_RADIO_DATA_TYPE_CO2 5
#define VV_RADIO_DATA_TYPE_THERMOSTAT_REFERENCE_VALUE 6
#define VV_RADIO_DATA_TYPE_WATERING_HUMIDTY 7
#define VV_RADIO_DATA_TYPE_WATERING_PUMP 8
#define VV_RADIO_DATA_TYPE_WATERING_WATER_LEVEL 9

// [BUFFER_TYPE NODE_ADDRESS DATA_TYPE NEW_VALUE]
#define VV_RADIO_TYPE             (0)
#define VV_RADIO_ADDRESS          (VV_RADIO_TYPE + sizeof(uint8_t))
#define VV_RADIO_DATA_TYPE        (VV_RADIO_ADDRESS + sizeof(uint64_t))
#define VV_RADIO_VALUE            (VV_RADIO_DATA_TYPE + sizeof(uint8_t))
#define VV_RADIO_MESSAGE_SIZE     (VV_RADIO_VALUE + sizeof(float))

struct vv_radio_single_float_packet {
    uint64_t device_address;
    uint8_t type;
    float value;
};

void vv_radio_listening_init();
void vv_radio_parse_incoming_buffer(size_t *length, uint8_t *buffer, struct vv_radio_single_float_packet *target);
void vv_radio_send_update(struct vv_radio_single_float_packet *source);

#endif
