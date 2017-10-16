
#ifndef VV_DISPLAY_H
#define VV_DISPLAY_H

#include <bc_common.h>
#include <bcl.h>

#define RADIO_VV_DISPLAY 0xf0

#define VV_DATA_COUNT 4
#define VV_VALUES_COUNT 15

typedef struct display_data {
    char* name;
    char* location;
    char *format;
    float_t values[VV_VALUES_COUNT];
};

struct vv_display_data_self {
    int8_t _actual_data_index;
    struct display_data actual_data[VV_DATA_COUNT];
} vv_display_data;

void vv_display_init();
void vv_lcd_page_render();
void vv_lcd_next_page();
void vv_lcd_prev_page();

void vv_display_parse_incoming_buffer(size_t *length, uint8_t *buffer);
void vv_display_send_update(uint64_t *device_address, uint8_t data_type_index, float *new_val);

#endif
