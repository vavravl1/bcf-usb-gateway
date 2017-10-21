
#ifndef VV_DISPLAY_H
#define VV_DISPLAY_H

#include <bc_common.h>
#include <bcl.h>

#define VV_PAGES_COUNT (VV_DATA_COUNT + 1)
#define VV_DATA_COUNT 4
#define VV_VALUES_COUNT 15

struct display_data {
    char* name;
    char* location;
    char *format;
    float values[VV_VALUES_COUNT];
};

struct display_controller {
    char* name;
    char* location;
    char *format;
    struct vv_thermostat_self* thermostat;
};

struct vv_display_self {
    int8_t actual_data_index;
    bc_led_t green_led;
    struct display_data actual_data[VV_DATA_COUNT];
    struct display_controller temperature_controller;
} vv_display;

void vv_display_init(struct vv_thermostat_self* _thermostat);
void vv_display_render();
void vv_display_next_page();
void vv_display_prev_page();

void vv_display_push_new_value(uint8_t index, float new_value);

void vv_display_parse_incoming_buffer(size_t *length, uint8_t *buffer);
void vv_display_send_update(uint64_t *device_address, uint8_t data_type_index, float *new_val);

void vv_display_set_actual_value(float new_actual_value);
void vv_display_set_reference_value(float new_reference_value);
float vv_display_get_reference_value();

#endif
