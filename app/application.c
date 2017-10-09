#include <application.h>
#include <usb_talk.h>
#include <bc_device_id.h>
#include <radio.h>
#include <sensors.h>

#include "watering.h"

#define UPDATE_INTERVAL 5000
#define APPLICATION_TASK_ID 0

static uint64_t my_device_address;
static bc_led_t led;
static bool led_state;
static bool radio_enrollment_mode;

static struct
{
    bc_tick_t next_update;
    bool mqtt;

} lcd;

static bc_module_relay_t relay_0_0;
static bc_module_relay_t relay_0_1;

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
static void lcd_button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
static void radio_event_handler(bc_radio_event_t event, void *event_param);
static void co2_event_handler(bc_module_co2_event_t event, void *event_param);
static void pir_event_handler(bc_module_pir_t *self, bc_module_pir_event_t event, void*event_param);

static void led_state_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void led_state_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void relay_state_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void relay_state_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void module_relay_state_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void module_relay_pulse(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void module_relay_state_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void lcd_text_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void lcd_screen_clear(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void led_strip_color_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void led_strip_brightness_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void led_strip_compound_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void led_strip_effect_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void led_strip_thermometer_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param);

static void info_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void nodes_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void nodes_purge(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void nodes_add(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void nodes_remove(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void scan_start(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void scan_stop(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void enrollment_start(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void enrollment_stop(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void automatic_pairing_start(uint64_t *device_address, usb_talk_payload_t *payload, void *param);
static void automatic_pairing_stop(uint64_t *device_address, usb_talk_payload_t *payload, void *param);

static void _radio_pub_state_set(uint8_t type, uint64_t *device_address, bool state);
static void _radio_pub_state_get(uint8_t type, uint64_t *device_address);

static uint8_t relay_0_number = 0;
static uint8_t relay_1_number = 1;

const usb_talk_subscribe_t subscribes[] = {
    {"led/-/state/set", led_state_set, NULL},
    {"led/-/state/get", led_state_get, NULL},
    {"relay/-/state/set", relay_state_set, NULL},
    {"relay/-/state/get", relay_state_get, NULL},
    {"relay/0:0/state/set", module_relay_state_set, &relay_0_number},
    {"relay/0:0/state/get", module_relay_state_get, &relay_0_number},
    {"relay/0:0/pulse/set", module_relay_pulse, &relay_0_number},
    {"relay/0:1/state/set", module_relay_state_set, &relay_1_number},
    {"relay/0:1/state/get", module_relay_state_get, &relay_1_number},
    {"relay/0:1/pulse/set", module_relay_pulse, &relay_1_number},
    {"lcd/-/text/set", lcd_text_set, NULL},
    {"lcd/-/screen/clear", lcd_screen_clear, NULL},
    {"led-strip/-/color/set", led_strip_color_set, NULL},
    {"led-strip/-/brightness/set", led_strip_brightness_set, NULL},
    {"led-strip/-/compound/set", led_strip_compound_set, NULL},
    {"led-strip/-/effect/set", led_strip_effect_set, NULL},
    {"led-strip/-/thermometer/set", led_strip_thermometer_set, NULL},
    {"/info/get", info_get, NULL},
    {"/nodes/get", nodes_get, NULL},
    {"/nodes/add", nodes_add, NULL},
    {"/nodes/remove", nodes_remove, NULL},
    {"/nodes/purge", nodes_purge, NULL},
    {"/scan/start", scan_start, NULL},
    {"/scan/stop", scan_stop, NULL},
    {"/enrollment/start", enrollment_start, NULL},
    {"/enrollment/stop", enrollment_stop, NULL},
    {"/automatic-pairing/start", automatic_pairing_start, NULL},
    {"/automatic-pairing/stop", automatic_pairing_stop, NULL}
};

void application_init(void)
{
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    usb_talk_init();
    usb_talk_subscribes(subscribes, sizeof(subscribes) / sizeof(usb_talk_subscribe_t));

    bc_radio_init();
    bc_radio_set_event_handler(radio_event_handler, NULL);
    bc_radio_listen();

    bc_module_power_init();

    memset(&lcd, 0, sizeof(lcd));

    bc_module_lcd_init(&_bc_module_lcd_framebuffer);
    bc_module_lcd_clear();
    bc_module_lcd_update();

    static bc_button_t button;
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    static bc_button_t lcd_left;
    bc_button_init_virtual(&lcd_left, BC_MODULE_LCD_BUTTON_LEFT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_left, lcd_button_event_handler, NULL);

    static bc_button_t lcd_right;
    bc_button_init_virtual(&lcd_right, BC_MODULE_LCD_BUTTON_RIGHT, bc_module_lcd_get_button_driver(), false);
    bc_button_set_event_handler(&lcd_right, lcd_button_event_handler, NULL);

    sensors_init_all(&my_device_address);

    //----------------------------

    bc_module_co2_init();
    bc_module_co2_set_update_interval(30000);
    bc_module_co2_set_event_handler(co2_event_handler, NULL);

    //----------------------------

    bc_module_relay_init(&relay_0_0, BC_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    bc_module_relay_init(&relay_0_1, BC_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);

    //----------------------------

    static bc_module_pir_t pir;
    bc_module_pir_init(&pir);
    bc_module_pir_set_event_handler(&pir, pir_event_handler, NULL);

    bc_led_set_mode(&led, BC_LED_MODE_OFF);
    led_state = false;
}

void application_task(void)
{
    bc_tick_t now = bc_tick_get();
    if (lcd.next_update < now)
    {
        bc_module_lcd_update();
        lcd.next_update = now + 500;
    }

    bc_scheduler_plan_current_relative(500);
}

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        static uint16_t event_count = 0;
        usb_talk_publish_push_button(&my_device_address, "-", &event_count);
        event_count++;
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_led_pulse(&led, 100);
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        if (radio_enrollment_mode)
        {
            radio_enrollment_mode = false;
            bc_radio_enrollment_stop();
            bc_led_set_mode(&led, BC_LED_MODE_OFF);
        }
        else{
            radio_enrollment_mode = true;
            bc_radio_enrollment_start();
            bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
        }
    }
}

static void lcd_button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

    if (event != BC_BUTTON_EVENT_CLICK)
    {
        return;
    }

    if (self->_channel.virtual_channel == BC_MODULE_LCD_BUTTON_LEFT)
    {
        static uint16_t event_left_count = 0;
        usb_talk_publish_push_button(&my_device_address, "lcd:left", &event_left_count);
        event_left_count++;
    }
    else
    {
        static uint16_t event_right_count = 0;
        usb_talk_publish_push_button(&my_device_address, "lcd:right", &event_right_count);
        event_right_count++;
    }
}

static void radio_event_handler(bc_radio_event_t event, void *event_param)
{
    (void) event_param;

    uint64_t peer_device_address = bc_radio_get_event_device_address();

    if (event == BC_RADIO_EVENT_ATTACH)
    {
        bc_led_pulse(&led, 1000);

        usb_talk_send_format("[\"/attach\", \"" USB_TALK_DEVICE_ADDRESS "\" ]\n", peer_device_address);
    }
    else if (event == BC_RADIO_EVENT_ATTACH_FAILURE)
    {
        bc_led_pulse(&led, 5000);

        usb_talk_send_format("[\"/attach-failure\", \"" USB_TALK_DEVICE_ADDRESS "\" ]\n", peer_device_address);
    }
    else if (event == BC_RADIO_EVENT_DETACH)
    {
        bc_led_pulse(&led, 1000);

        usb_talk_send_format("[\"/detach\", \"" USB_TALK_DEVICE_ADDRESS "\" ]\n", peer_device_address);
    }
    else if (event == BC_RADIO_EVENT_INIT_DONE)
    {
        my_device_address = bc_radio_get_device_address();
    }
    else if (event == BC_RADIO_EVENT_SCAN_FIND_DEVICE)
    {
        usb_talk_send_format("[\"/scan\", \"" USB_TALK_DEVICE_ADDRESS "\" ]\n", peer_device_address);
    }
}

void bc_radio_on_push_button(uint64_t *peer_device_address, uint16_t *event_count)
{
    usb_talk_publish_push_button(peer_device_address, "-", event_count);
}

void bc_radio_on_thermometer(uint64_t *peer_device_address, uint8_t *i2c, float *temperature)
{
    (void) peer_device_address;

    usb_talk_publish_thermometer(peer_device_address, i2c, temperature);
}

void bc_radio_on_humidity(uint64_t *peer_device_address, uint8_t *i2c, float *percentage)
{
    (void) peer_device_address;

    usb_talk_publish_humidity_sensor(peer_device_address, i2c, percentage);
}

void bc_radio_on_lux_meter(uint64_t *peer_device_address, uint8_t *i2c, float *illuminance)
{
    (void) peer_device_address;

    usb_talk_publish_lux_meter(peer_device_address, i2c, illuminance);
}

void bc_radio_on_barometer(uint64_t *peer_device_address, uint8_t *i2c, float *pressure, float *altitude)
{
    (void) peer_device_address;

    usb_talk_publish_barometer(peer_device_address, i2c, pressure, altitude);
}

void bc_radio_on_co2(uint64_t *peer_device_address, float *concentration)
{
    (void) peer_device_address;

    usb_talk_publish_co2_concentation(peer_device_address, concentration);
}

void bc_radio_on_battery(uint64_t *peer_device_address, uint8_t *format, float *voltage)
{
    usb_talk_send_format("[\"%012llx/battery/%s/voltage\", %.2f]\n",
            *peer_device_address,
            *format == 0 ? "standard" : "mini",
            *voltage);
}

void bc_radio_on_buffer(uint64_t *peer_device_address, uint8_t *buffer, size_t *length)
{
    if (*length < 1)
    {
        return;
    }

    switch (buffer[0]) {
        case RADIO_USER_TOPIC_BOOL:
        {
            if (*length < (1 + 1 + 1))
            {
                return;
            }
            usb_talk_publish_bool(peer_device_address, (char *)buffer + 2, (bool *)(buffer + 1));
            break;
        }
        case RADIO_USER_TOPIC_INT:
        {
            if (*length < (1 + 4 + 1))
            {
                return;
            }
            int value;
            memcpy(&value, buffer + 1, sizeof(int));
            usb_talk_publish_int(peer_device_address, (char *)buffer + 5, &value);
            break;
        }
        case RADIO_USER_TOPIC_FLOAT:
        {
            if (*length < (1 + 4 + 1))
            {
                return;
            }
            float value;
            memcpy(&value, buffer + 1, sizeof(value));
            usb_talk_publish_float(peer_device_address, (char *)buffer + 5, &value);
            break;
        }
        default:
        {
            break;
        }
    }

    if (*length == 2)
    {
        switch (buffer[0])
        {
            case RADIO_LED:
            {
                bool state = buffer[1];
                usb_talk_publish_led(peer_device_address, &state);
                break;
            }
            case RADIO_RELAY_0:
            {
                uint8_t number = 0;
                bc_module_relay_state_t state = buffer[1];
                usb_talk_publish_module_relay(peer_device_address, &number, &state);
                break;
            }
            case RADIO_RELAY_1:
            {
                uint8_t number = 1;
                bc_module_relay_state_t state = buffer[1];
                usb_talk_publish_module_relay(peer_device_address, &number, &state);
                break;
            }
            case RADIO_RELAY_POWER:
            {
                bool state = buffer[1];
                usb_talk_publish_relay(peer_device_address, &state);
                break;
            }
            case RADIO_HUMIDITY:
            {
                uint8_t watering_humidity = buffer[1];
	        usb_talk_publish_watering_humidity(peer_device_address, watering_humidity);
                break;
            }
            case RADIO_PUMP:
            {
                uint8_t watering_pump = buffer[1];
	        usb_talk_publish_watering_pump(peer_device_address, watering_pump);
                break;
            }	    
            case RADIO_WATER_LEVEL:
            {
                uint8_t watering_water_level = buffer[1];
	        usb_talk_publish_watering_water_level(peer_device_address, watering_water_level);
                break;
            }	
            default:
            {
                break;
            }
        }
    }
    else if (*length == 3)
    {
        switch (buffer[0])
        {
            case RADIO_PIR:
            {
                uint16_t event_count;
                memcpy(&event_count, buffer + 1, sizeof(event_count));
                usb_talk_publish_event_count(peer_device_address, "pir", &event_count);
                break;
            }
            case RADIO_FLOOD_DETECTOR:
            {
                usb_talk_publish_flood_detector(peer_device_address, (char *) (buffer + 1), (bool *) (buffer + 2));
                break;
            }
            case RADIO_LCD_BUTTON_LEFT:
            {
                uint16_t event_count;
                memcpy(&event_count, buffer + 1, sizeof(event_count));
                usb_talk_publish_push_button(peer_device_address, "lcd:left", &event_count);
                break;
            }
            case RADIO_LCD_BUTTON_RIGHT:
            {
                uint16_t event_count;
                memcpy(&event_count, buffer + 1, sizeof(event_count));
                usb_talk_publish_push_button(peer_device_address, "lcd:right", &event_count);
                break;
            }
            case RADIO_ACCELEROMETER_ALERT:
            {
                uint16_t event_count;
                memcpy(&event_count, buffer + 1, sizeof(event_count));
                usb_talk_publish_event_count(peer_device_address, "accelerometer", &event_count);
                break;
            }
            case RADIO_MAGNET_SWITCH_STATE:
            {
                usb_talk_publish_complex_bool(peer_device_address, "magnet-switch", buffer[1] == RADIO_CHANNEL_A ? "a" : "b", "state", (bool *) &buffer[2]);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if (*length == 5)
    {
        switch (buffer[0])
        {
            case RADIO_THERMOSTAT_SET_POINT_TEMPERATURE:
            {
                float temperature;
                memcpy(&temperature, buffer + 1, sizeof(temperature));
                usb_talk_publish_float(peer_device_address, "thermostat/set-point/temperature", &temperature);
                break;
            }
            default:
            {
                break;
            }

        }
    }
    else if (*length == 13)
    {
        switch (buffer[0])
        {
            case RADIO_ACCELEROMETER_ACCELERATION:
            {
                float x_axis, y_axis, z_axis;
                memcpy(&x_axis, buffer + 1, sizeof(x_axis));
                memcpy(&y_axis, buffer + 1 + sizeof(x_axis), sizeof(y_axis));
                memcpy(&z_axis, buffer + 1 + sizeof(x_axis) + sizeof(y_axis), sizeof(z_axis));
                usb_talk_publish_accelerometer_acceleration(peer_device_address, &x_axis, &y_axis, &z_axis);
                break;
            }
            default:
            {
                break;
            }

        }
    }
}

void co2_event_handler(bc_module_co2_event_t event, void *event_param)
{
    (void) event_param;
    float value;

    if (event == BC_MODULE_CO2_EVENT_UPDATE)
    {
        if (bc_module_co2_get_concentration(&value))
        {
            usb_talk_publish_co2_concentation(&my_device_address, &value);
        }
    }
}

static void pir_event_handler(bc_module_pir_t *self, bc_module_pir_event_t event, void*event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_MODULE_PIR_EVENT_MOTION)
    {
        static uint16_t event_count = 0;
        event_count++;
        usb_talk_publish_event_count(&my_device_address, "pir", &event_count);
    }
}

static void led_state_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;
    bool state;

    if (!usb_talk_payload_get_bool(payload, &state))
    {
        return;
    }

    if (my_device_address == *device_address)
    {
        led_state = state;

        bc_led_set_mode(&led, led_state ? BC_LED_MODE_ON : BC_LED_MODE_OFF);

        usb_talk_publish_led(&my_device_address, &led_state);
    }
    else
    {
        _radio_pub_state_set(RADIO_LED_SET, device_address, state);
    }
}

static void led_state_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    (void) param;

    if (my_device_address == *device_address)
    {
        usb_talk_publish_led(&my_device_address, &led_state);
    }
    else
    {
        _radio_pub_state_get(RADIO_LED_GET, device_address);
    }
}

static void relay_state_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;
    bool state;

    if (!usb_talk_payload_get_bool(payload, &state))
    {
        return;
    }

    if (my_device_address == *device_address)
    {
        bc_module_power_relay_set_state(state);

        usb_talk_publish_relay(&my_device_address, &state);
    }
    else
    {
        _radio_pub_state_set(RADIO_RELAY_POWER_SET, device_address, state);
    }
}

static void relay_state_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    (void) param;

    if (my_device_address == *device_address)
    {
        bool state = bc_module_power_relay_get_state();

        usb_talk_publish_relay(&my_device_address, &state);
    }
    else
    {
        _radio_pub_state_get(RADIO_RELAY_POWER_GET, device_address);
    }

}

static void module_relay_state_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) payload;

    bool state;
    uint8_t *number = (uint8_t *) param;

    if (!usb_talk_payload_get_bool(payload, &state))
    {
        return;
    }

    if (my_device_address == *device_address)
    {
        bc_module_relay_set_state(*number == 0 ? &relay_0_0 : &relay_0_1, state);
        bc_module_relay_state_t relay_state = state ? BC_MODULE_RELAY_STATE_TRUE : BC_MODULE_RELAY_STATE_FALSE;
        usb_talk_publish_module_relay(&my_device_address, number, &relay_state);
    }
    else
    {
        _radio_pub_state_set(*number == 0 ? RADIO_RELAY_0_SET : RADIO_RELAY_1_SET, device_address, state);
    }
}

static void module_relay_pulse(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{

    int duration;
    bool direction;
    uint8_t *number = (uint8_t *) param;

    if (!usb_talk_payload_get_key_int(payload, "duration", &duration))
    {
        duration = 500;
    }

    if (!usb_talk_payload_get_key_bool(payload, "direction", &direction))
    {
        direction = true;
    }

    if (my_device_address == *device_address)
    {
        bc_module_relay_pulse(*number == 0 ? &relay_0_0 : &relay_0_1, direction, duration);
    }
    else
    {
        uint8_t buffer[1 + sizeof(uint64_t) + 1 + 4]; // HEAD + ADDRESS + DIRECTION + DURATION(4)
        buffer[0] = (*number == 0) ? RADIO_RELAY_0_PULSE_SET : RADIO_RELAY_1_PULSE_SET;
        memcpy(buffer + 1, device_address, sizeof(uint64_t));
        buffer[sizeof(uint64_t) + 1] = (uint8_t) direction;
        memcpy(&buffer[sizeof(uint64_t) + 2], &duration, sizeof(uint32_t));

        bc_radio_pub_buffer(buffer, sizeof(buffer));
    }
}

static void module_relay_state_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    uint8_t *number = (uint8_t *) param;

    if (my_device_address == *device_address)
    {

        bc_module_relay_state_t state = bc_module_relay_get_state(*number == 0 ? &relay_0_0 : &relay_0_1);

        usb_talk_publish_module_relay(&my_device_address, number, &state);
    }
    else
    {
        _radio_pub_state_get(*number == 0 ? RADIO_RELAY_0_GET : RADIO_RELAY_1_GET, device_address);
    }

}

static void lcd_text_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;

    int x;
    int y;
    int font_size;
    bool color;
    char text[32];
    size_t length = sizeof(text);

    memset(text, 0, length);
    if (!usb_talk_payload_get_key_int(payload, "x", &x))
    {
        return;
    }
    if (!usb_talk_payload_get_key_int(payload, "y", &y))
    {
        return;
    }
    if (!usb_talk_payload_get_key_string(payload, "text", text, &length))
    {
        return;
    }

    if (!lcd.mqtt)
    {
        bc_module_lcd_clear();
        lcd.mqtt = true;
    }

    if (!usb_talk_payload_get_key_int(payload, "font", &font_size))
    {
        font_size = 15;
    }

    if (!usb_talk_payload_get_key_bool(payload, "color", &color))
    {
        color = true;
    }

    if (my_device_address == *device_address)
    {
        switch (font_size)
        {
            case 11:
            {
                bc_module_lcd_set_font(&bc_font_ubuntu_11);
                break;
            }
            case 13:
            {
                bc_module_lcd_set_font(&bc_font_ubuntu_13);
                break;
            }
            case 15:
            {
                bc_module_lcd_set_font(&bc_font_ubuntu_15);
                break;
            }
            case 24:
            {
                bc_module_lcd_set_font(&bc_font_ubuntu_24);
                break;
            }
            case 28:
            {
                bc_module_lcd_set_font(&bc_font_ubuntu_28);
                break;
            }
            case 33:
            {
                bc_module_lcd_set_font(&bc_font_ubuntu_33);
                break;
            }
            default:
            {
                bc_module_lcd_set_font(&bc_font_ubuntu_15);
                break;
            }
        }

        bc_module_lcd_draw_string(x, y, text, color);
    }
    else
    {
        uint8_t buffer[1 + sizeof(uint64_t) + 5 + 32]; // HEAD + ADDRESS + X + Y + FONT_SIZE + LENGTH + TEXT
        buffer[0] = RADIO_LCD_TEXT_SET;
        memcpy(buffer + 1, device_address, sizeof(uint64_t));
        buffer[sizeof(uint64_t) + 1] = (uint8_t) x;
        buffer[sizeof(uint64_t) + 2] = (uint8_t) y;
        buffer[sizeof(uint64_t) + 3] = (uint8_t) font_size;
        buffer[sizeof(uint64_t) + 4] = (uint8_t) color;
        buffer[sizeof(uint64_t) + 5] = (uint8_t) length;
        memcpy(buffer + sizeof(uint64_t) + 6, text, length + 1);

        bc_radio_pub_buffer(buffer, 1 + sizeof(uint64_t) + 4 + length + 1);
    }
}

static void lcd_screen_clear(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    (void) param;

    if (my_device_address == *device_address)
    {
        bc_module_lcd_clear();
    }
    else
    {
        uint8_t buffer[1 + sizeof(uint64_t)];
        buffer[0] = RADIO_LCD_SCREEN_CLEAR;
        memcpy(buffer + 1, device_address, sizeof(uint64_t));
        bc_radio_pub_buffer(buffer, sizeof(buffer));
    }
}

static void led_strip_color_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;
    uint8_t buffer[1 + sizeof(uint64_t) + 4];
    buffer[0] = RADIO_LED_STRIP_COLOR_SET;
    memcpy(buffer + 1, device_address, sizeof(uint64_t));

    char str[12];
    size_t length = sizeof(str);
    if (!usb_talk_payload_get_string(payload, str, &length))
    {
        return;
    }

    if (((length != 7) && (length != 11)) || (str[0] != '#'))
    {
        return;
    }

    buffer[sizeof(uint64_t) + 1] = usb_talk_hex_to_u8(str + 1);
    buffer[sizeof(uint64_t) + 2] = usb_talk_hex_to_u8(str + 3);
    buffer[sizeof(uint64_t) + 3] = usb_talk_hex_to_u8(str + 5);
    buffer[sizeof(uint64_t) + 4] = (length == 11) ? usb_talk_hex_to_u8(str + 8) : 0x00;

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

static void led_strip_brightness_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;
    int value;
    if (!usb_talk_payload_get_int(payload, &value))
    {
        return;
    }

    if ((value < 0) || value > 100)
    {
        return;
    }

    uint8_t buffer[1 + sizeof(uint64_t) + 1];
    buffer[0] = RADIO_LED_STRIP_BRIGHTNESS_SET;
    memcpy(buffer + 1, device_address, sizeof(uint64_t));
    buffer[sizeof(uint64_t) + 1] = value;

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

static void led_strip_compound_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;
    uint8_t buffer[64 - 9];
    buffer[0] = RADIO_LED_STRIP_COMPOUND_SET;
    memcpy(buffer + 1, device_address, sizeof(uint64_t));

    int count_sum = 0;
    size_t length;

    do
    {
        length = sizeof(buffer) - sizeof(uint64_t) - 2;

        buffer[sizeof(uint64_t) + 1] = count_sum;

        usb_talk_payload_get_compound_buffer(payload, buffer + sizeof(uint64_t) + 2, &length, &count_sum);

        bc_radio_pub_buffer(buffer, length + sizeof(uint64_t) + 2);

    } while ((length == 45) && (count_sum < 255));

}

static void led_strip_effect_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;
    uint8_t buffer[1 + sizeof(uint64_t) + 1 + sizeof(uint16_t) + sizeof(uint32_t)];
    int type;
    int int_wait;
    uint16_t wait;

    buffer[0] = RADIO_LED_STRIP_EFFECT_SET;
    memcpy(buffer + 1, device_address, sizeof(uint64_t));

    if (!usb_talk_payload_get_key_enum(payload, "type", &type, "test", "rainbow", "rainbow-cycle", "theater-chase-rainbow", "color-wipe", "theater-chase"))
    {
        return;
    }

    buffer[1 + sizeof(uint64_t)] = (uint8_t) type;

    if (type > RADIO_LED_STRIP_EFFECT_TYPE_TEST)
    {
        if (!usb_talk_payload_get_key_int(payload, "wait", &int_wait))
        {
            return;
        }

        if (int_wait < 0)
        {
            return;
        }

        wait = (uint16_t) int_wait;

        memcpy(buffer + 1 + sizeof(uint64_t) + 1, &wait, sizeof(wait));
    }

    if (type > RADIO_LED_STRIP_EFFECT_TYPE_THEATER_CHASE_RAINBOW)
    {
        char str[12];
        size_t length = sizeof(str);
        if (!usb_talk_payload_get_key_string(payload, "color", str, &length))
        {
            return;
        }

        if (((length != 7) && (length != 11)) || (str[0] != '#'))
        {
            return;
        }

        buffer[1 + sizeof(uint64_t) + 1 + sizeof(uint16_t) + 3] = usb_talk_hex_to_u8(str + 1);
        buffer[1 + sizeof(uint64_t) + 1 + sizeof(uint16_t) + 2] = usb_talk_hex_to_u8(str + 3);
        buffer[1 + sizeof(uint64_t) + 1 + sizeof(uint16_t) + 1] = usb_talk_hex_to_u8(str + 5);
        buffer[1 + sizeof(uint64_t) + 1 + sizeof(uint16_t) + 0] = (length == 11) ? usb_talk_hex_to_u8(str + 8) : 0x00;
    }

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

static void led_strip_thermometer_set(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) param;

    float temperature;
    int min;
    int max;
    uint8_t buffer[1 + sizeof(uint64_t) + sizeof(float) + 1 + 1];

    if (!usb_talk_payload_get_key_float(payload, "temperature", &temperature))
    {
        return;
    }

    if (!usb_talk_payload_get_key_int(payload, "min", &min) || (min > 127) || (min < -128))
    {
        return;
    }

    if (!usb_talk_payload_get_key_int(payload, "max", &max) || (max > 127) || (max < -128))
    {
        return;
    }

    buffer[0] = RADIO_LED_STRIP_THERMOMETER_SET;
    memcpy(buffer + 1, device_address, sizeof(uint64_t));

    memcpy(buffer + 1 + sizeof(uint64_t), &temperature, sizeof(temperature));

    buffer[1 + sizeof(uint64_t) + sizeof(uint32_t)] = (int8_t) min;
    buffer[1 + sizeof(uint64_t) + sizeof(uint32_t) + 1] = (int8_t) max;

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

static void info_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) param;
    (void) payload;

    usb_talk_send_format("[\"/info\", {\"address\": \"" USB_TALK_DEVICE_ADDRESS "\", \"firmware\": \"bcf-usb-gateway\"}]\n", my_device_address);
}

static void nodes_get(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) param;
    (void) payload;

    uint64_t peer_devices_address[BC_RADIO_MAX_DEVICES];

    bc_radio_get_peer_devices_address(peer_devices_address, BC_RADIO_MAX_DEVICES);

    usb_talk_publish_nodes(peer_devices_address, BC_RADIO_MAX_DEVICES);
}


void _radio_node(usb_talk_payload_t *payload, bool (*call)(uint64_t))
{
    char tmp[13];
    size_t length = sizeof(tmp);

    if (!usb_talk_payload_get_string(payload, tmp, &length))
    {
        return;
    }

    if (length == 12)
    {
        uint64_t device_address = 0;

        if (sscanf(tmp, "%012llx/", &device_address))
        {
            call(device_address);
        }
    }
}

static void nodes_add(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) param;
    _radio_node(payload, bc_radio_peer_device_add);
}

static void nodes_remove(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) param;
    _radio_node(payload, bc_radio_peer_device_remove);
}

static void nodes_purge(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) payload;
    (void) param;

    bc_radio_peer_device_purge_all();
}

static void scan_start(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) payload;
    (void) param;

    bc_radio_scan_start();
}

static void scan_stop(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) payload;
    (void) param;

    bc_radio_scan_stop();
}


static void enrollment_start(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) payload;
    (void) param;

    radio_enrollment_mode = true;

    bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);

    bc_radio_enrollment_start();
}

static void enrollment_stop(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) payload;
    (void) param;

    radio_enrollment_mode = false;

    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    bc_radio_enrollment_stop();
}

static void automatic_pairing_start(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) payload;
    (void) param;

    bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);

    bc_radio_automatic_pairing_start();
}

static void automatic_pairing_stop(uint64_t *device_address, usb_talk_payload_t *payload, void *param)
{
    (void) device_address;
    (void) payload;
    (void) param;

    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    bc_radio_automatic_pairing_stop();
}

static void _radio_pub_state_set(uint8_t type, uint64_t *device_address, bool state)
{
    uint8_t buffer[1 + sizeof(uint64_t) + 1];

    buffer[0] = type;
    memcpy(buffer + 1, device_address, sizeof(uint64_t));
    buffer[sizeof(uint64_t) + 1] = state;

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}

static void _radio_pub_state_get(uint8_t type, uint64_t *device_address)
{
    uint8_t buffer[1 + sizeof(uint64_t)];

    buffer[0] = type;
    memcpy(buffer + 1, device_address, sizeof(uint64_t));

    bc_radio_pub_buffer(buffer, sizeof(buffer));
}
