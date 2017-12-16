
#include "vv_radio.h"
#include "vv_display.h"
#include "vv_thermostat.h"

#include <bc_common.h>
#include <bcl.h>

void _vv_radio_listen_task();
bc_scheduler_task_id_t _vv_radio_listen_task_id;

void _vv_radio_sleep_task();
bc_scheduler_task_id_t _vv_radio_sleep_task_id;


void vv_radio_listening_init() {
    _vv_radio_listen_task_id = bc_scheduler_register(_vv_radio_listen_task, NULL, 0);
    _vv_radio_sleep_task_id = bc_scheduler_register(_vv_radio_sleep_task, NULL, BC_TICK_INFINITY);
}

void vv_radio_parse_incoming_buffer(size_t *length, uint8_t *buffer) {
    if (*length != VV_RADIO_MESSAGE_SIZE) { 
	return;
    }
    uint8_t data_type_index = buffer[VV_RADIO_DATA_TYPE_INDEX];
    float new_val;
    memcpy(&new_val, buffer + VV_RADIO_NEW_VAL, sizeof(float));

    if(data_type_index < VV_DATA_COUNT) {
	vv_display_push_new_value(data_type_index, new_val);	    
    } else if(data_type_index == VV_RADIO_DATA_TYPE_THERMOSTAT_REFERENCE_VALUE) {
	vv_thermostat_set_reference_value(&vv_thermostat, &new_val);
    }

    vv_display_render();
}

void vv_radio_send_update(uint64_t *device_address, uint8_t data_type_index, float *new_val) {
    uint8_t buffer[VV_RADIO_MESSAGE_SIZE];    
    buffer[VV_RADIO_TYPE] = VV_RADIO_THERMOSTAT;
    memcpy(buffer + VV_RADIO_ADDRESS, device_address, sizeof(uint64_t));    
    buffer[VV_RADIO_DATA_TYPE_INDEX] = data_type_index;
    memcpy(buffer + VV_RADIO_NEW_VAL, new_val, sizeof(*new_val));    

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}


void _vv_radio_listen_task() {
    bc_radio_listen();
    uint16_t one = 1;
    bc_radio_pub_push_button(&one);
    bc_scheduler_plan_relative(_vv_radio_sleep_task_id, 1000);    
}

void _vv_radio_sleep_task() {
    bc_radio_sleep();    
    bc_scheduler_plan_relative(_vv_radio_listen_task_id, 10000);    
}
