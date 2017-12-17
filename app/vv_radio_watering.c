
#include "vv_radio_watering.h"
#include <usb_talk.h>

void process_incoming_packet(struct vv_radio_single_float_packet *packet) {
    switch(packet->type) {
	case VV_RADIO_DATA_TYPE_WATERING_HUMIDTY:
	    usb_talk_publish_watering_humidity(&packet -> device_address, packet -> value);
	    break;
	case VV_RADIO_DATA_TYPE_WATERING_PUMP:
	    usb_talk_publish_watering_pump(&packet -> device_address, packet -> value);
	    break;
	case VV_RADIO_DATA_TYPE_WATERING_WATER_LEVEL:
	    usb_talk_publish_watering_water_level(&packet -> device_address, packet -> value);
	    break;
	default: break;
    }
}

