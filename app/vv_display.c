

#include "vv_display.h"
#include <math.h>

#define _VV_GRAPH_TOP 80
#define _VV_GRAPH_BOTTOM 110
#define _VV_GRAPH_SIDE 10

//struct display_data actual_data[] = {
//    { .name = "L1 power", .location = "House", .format="%.1f kW", .values = {0.7, 1.1, 0.5, -0.1, -0.8} },
//    { .name = "Temperature", .location = "Living room", .format="%.1f \xb0", .values = {19.3, 18.7, 19.2, 19.6, 20.3} },
//};
//
//size_t _actual_data_size = 2;
//int8_t _actual_data_index = 0;
//

float _find_min() {
    float min = 10e10;
    for(uint8_t i = 0; i < VV_VALUES_COUNT; i++) {
	if(min > vv_display_data.actual_data[vv_display_data._actual_data_index].values[i]) {
	    min = vv_display_data.actual_data[vv_display_data._actual_data_index].values[i];
	}
    }
    return min;
}

float _find_max() {
    float max = -10e10;
    for(uint8_t i = 0; i < VV_VALUES_COUNT; i++) {
	if(max < vv_display_data.actual_data[vv_display_data._actual_data_index].values[i]) {
	    max = vv_display_data.actual_data[vv_display_data._actual_data_index].values[i];
	}
    }
    return max;
}

int _left_intend_to_center(char *str, uint8_t char_size) {
    return fmax(1, (128 - strlen(str) * char_size) / 2);
}

void _draw_header() {
    bc_module_lcd_set_font(&bc_font_ubuntu_15);
    bc_module_lcd_draw_string(_left_intend_to_center(vv_display_data.actual_data[vv_display_data._actual_data_index].name, 7), 5, vv_display_data.actual_data[vv_display_data._actual_data_index].name, true);

    bc_module_lcd_set_font(&bc_font_ubuntu_24);
    bc_module_lcd_draw_string(_left_intend_to_center(vv_display_data.actual_data[vv_display_data._actual_data_index].location, 11), 20, vv_display_data.actual_data[vv_display_data._actual_data_index].location, true);
}

void _draw_last_value() {
    char str[32];
    bc_module_lcd_set_font(&bc_font_ubuntu_33);
    snprintf(str, sizeof(str), vv_display_data.actual_data[vv_display_data._actual_data_index].format, vv_display_data.actual_data[vv_display_data._actual_data_index].values[VV_VALUES_COUNT - 1]);

    bc_module_lcd_draw_string(_left_intend_to_center(str, 14), 45, str, true); 
}

void _draw_graph() {
    float min = _find_min();
    float max = _find_max();

    //(19.3, 20.3) -> (115 -> 80)
    //(19.3, 20.3) -> (35 -> 0)

    //19.3*x + y = 35
    //20.3*x + y = 0

    // x = -35 / (max - min)
    // y = - x * max

    float_t x = - (_VV_GRAPH_BOTTOM - _VV_GRAPH_TOP) / (max - min);
    float_t y = - x * max + _VV_GRAPH_TOP;

    int left_step = (128 - (2 * _VV_GRAPH_SIDE)) / VV_VALUES_COUNT;

    int last_top = x * vv_display_data.actual_data[vv_display_data._actual_data_index].values[0] + y;
    int last_left = _VV_GRAPH_SIDE;

    for(uint8_t i = 0; i < VV_VALUES_COUNT; i++) {
	int top = (int)(x * vv_display_data.actual_data[vv_display_data._actual_data_index].values[i] + y);
	int left = _VV_GRAPH_SIDE + left_step * i;

        bc_module_lcd_set_font(&bc_font_ubuntu_11);
        bc_module_lcd_draw_char(left - 2, top - 6, 'o', true);
	

	bc_module_lcd_draw_line(last_left, last_top, left, top, true);
	last_top = top;
	last_left = left;
    }
}

void _draw_footer() {
    float max = _find_max();    
    char max_str[8];
    snprintf(max_str, sizeof(max_str), vv_display_data.actual_data[vv_display_data._actual_data_index].format, max);    

    float min = _find_min();
    char min_str[8];
    snprintf(min_str, sizeof(min_str), vv_display_data.actual_data[vv_display_data._actual_data_index].format, min);
    
    char str[64];
    snprintf(str, sizeof(str), "%s / %s", min_str, max_str);

    bc_module_lcd_set_font(&bc_font_ubuntu_15);    
    bc_module_lcd_draw_string(_left_intend_to_center(str, 7), 113, str, true);     
}

void vv_display_init() {
    vv_display_data._actual_data_index = 0;

    vv_display_data.actual_data[0].name = "L1 power [kW]";
    vv_display_data.actual_data[0].location = "House";
    vv_display_data.actual_data[0].format = "%.2f";
    for(uint8_t i = 0; i < VV_VALUES_COUNT; i++) {
	vv_display_data.actual_data[0].values[i] = 0;
    }
    
    vv_display_data.actual_data[1].name = "Temperature [\xb0]";
    vv_display_data.actual_data[1].location = "Living room";
    vv_display_data.actual_data[1].format = "%.2f";
    for(uint8_t i = 0; i < VV_VALUES_COUNT; i++) {
	vv_display_data.actual_data[1].values[i] = 0;
    }

    vv_display_data.actual_data[2].name = "Temperature [\xb0]";
    vv_display_data.actual_data[2].location = "Terrace";
    vv_display_data.actual_data[2].format = "%.2f";
    for(uint8_t i = 0; i < VV_VALUES_COUNT; i++) {
	vv_display_data.actual_data[2].values[i] = 0;
    }    

    vv_display_data.actual_data[3].name = "CO2 [ppm]";
    vv_display_data.actual_data[3].location = "Upstairs";
    vv_display_data.actual_data[3].format = "%.0f";
    for(uint8_t i = 0; i < VV_VALUES_COUNT; i++) {
	vv_display_data.actual_data[3].values[i] = 0;
    }        
}

void _vv_display_push_new_value(uint8_t index, float_t new_value) {
    for(uint8_t i = 0; i < VV_VALUES_COUNT - 1; i++) {
	vv_display_data.actual_data[index].values[i] = vv_display_data.actual_data[index].values[i + 1];
    }
    vv_display_data.actual_data[index].values[VV_VALUES_COUNT - 1] = new_value;
}

void vv_lcd_page_render() {
    bc_module_core_pll_enable();
    bc_module_lcd_clear();

    _draw_header();
    _draw_last_value();
    _draw_graph();
    _draw_footer();
    
    bc_module_core_pll_disable();
    bc_module_lcd_update();    
}

void vv_lcd_next_page() {
    vv_display_data._actual_data_index = (vv_display_data._actual_data_index + 1) % VV_DATA_COUNT;
}

void vv_lcd_prev_page() {
    vv_display_data._actual_data_index--;
    if(vv_display_data._actual_data_index < 0)  vv_display_data._actual_data_index = VV_DATA_COUNT - 1;    
}

#define _VV_DISPLAY_RADIO_TYPE             (0)
#define _VV_DISPLAY_RADIO_ADDRESS          (_VV_DISPLAY_RADIO_TYPE + sizeof(uint8_t))
#define _VV_DISPLAY_RADIO_DATA_TYPE_INDEX  (_VV_DISPLAY_RADIO_ADDRESS + sizeof(uint64_t))
#define _VV_DISPLAY_RADIO_NEW_VAL          (_VV_DISPLAY_RADIO_DATA_TYPE_INDEX + sizeof(uint8_t))
#define _VV_DISPLAY_RADIO_MESSAGE_SIZE     (_VV_DISPLAY_RADIO_NEW_VAL + sizeof(float))

void vv_display_parse_incoming_buffer(size_t *length, uint8_t *buffer) {
    if (*length != _VV_DISPLAY_RADIO_MESSAGE_SIZE) { //BUFFER_TYPE ADDRESS DATA_TYPE NEW_VALUE
	return;
    }
    uint8_t data_type_index = buffer[_VV_DISPLAY_RADIO_DATA_TYPE_INDEX];
    float new_val;
    memcpy(&new_val, buffer + _VV_DISPLAY_RADIO_NEW_VAL, sizeof(float));
    _vv_display_push_new_value(data_type_index, new_val);	    

    vv_lcd_page_render();
}

void vv_display_send_update(uint64_t *device_address, uint8_t data_type_index, float *new_val) {
    uint8_t buffer[_VV_DISPLAY_RADIO_MESSAGE_SIZE];    
    buffer[_VV_DISPLAY_RADIO_TYPE] = RADIO_VV_DISPLAY;
    memcpy(buffer + _VV_DISPLAY_RADIO_ADDRESS, device_address, sizeof(uint64_t));    
    buffer[_VV_DISPLAY_RADIO_DATA_TYPE_INDEX] = data_type_index;
    memcpy(buffer + _VV_DISPLAY_RADIO_NEW_VAL, new_val, sizeof(*new_val));    

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

