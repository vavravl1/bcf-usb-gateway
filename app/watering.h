#ifndef WATERING_H
#define WATERING_H

#include <bc_gpio.h>
#include <bc_scheduler.h>

#define PORT_WATER_LEVEL_POWER BC_GPIO_P9
#define PORT_WATER_LEVEL_INPUT BC_GPIO_P2
#define PORT_HUMIDITY_MEASURE BC_ADC_CHANNEL_A3
#define PORT_HUMIDITY_POWER BC_GPIO_P4
#define PORT_WATER_PUMP BC_GPIO_P5


struct {
    bc_scheduler_task_id_t _watering_process_task_id;
    bc_scheduler_task_id_t _measure_humidity_task_id;
    bc_scheduler_task_id_t _measure_water_level_task_id;
    bc_scheduler_task_id_t _stop_measuring_water_level_task_id;
    bc_scheduler_task_id_t _stop_water_pump_task_id;
    bc_scheduler_task_id_t _stop_measuring_humidity_task_id;
    uint8_t _measured_humidity;
    uint8_t _measured_water_level;
} vv_watering_self;

void vv_init_watering();
void vv_start_water_pump();
void vv_log_to_usb(char* message, const char * format, ...);


#define RADIO_HUMIDITY 0xb0
#define RADIO_PUMP 0xb1
#define RADIO_WATER_LEVEL 0xb2

#endif

