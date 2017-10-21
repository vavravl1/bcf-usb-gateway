
#include "vv_thermostat.h"
#include <bcl.h>

void _vv_thermostat_task();

void vv_thermostat_init(bc_module_relay_t* __relay) {
    vv_thermostat.relay = __relay;
    vv_thermostat.actual_value = 0;
    vv_thermostat.reference_value = 20;
    vv_thermostat.vv_thermostat_task_id = bc_scheduler_register(_vv_thermostat_task, (void*)&vv_thermostat, VV_THERMOSTAT_TASK_DELAY);
}

bool vv_thermostat_get_actual_state(struct vv_thermostat_self *self) {
    return self->actual_state;
}

void vv_thermostat_set_actual_state(struct vv_thermostat_self *self, bool new_state) {
    self->actual_state = new_state;
    bc_module_relay_set_state(self->relay, vv_thermostat_get_actual_state(self));
}

float* vv_thermostat_get_actual_value(struct vv_thermostat_self *self) {
    return & self->actual_value;
}

void vv_thermostat_set_actual_value(struct vv_thermostat_self *self, float* new_actual_value) {
    self->actual_value = *new_actual_value;
}

float* vv_thermostat_get_reference_value(struct vv_thermostat_self *self) {
    return & self->reference_value;
}

void vv_thermostat_set_reference_value(struct vv_thermostat_self *self, float* new_reference_value) {
    self->reference_value = *new_reference_value;
}

void _vv_thermostat_task(void* param) {
    struct vv_thermostat_self *self = (struct vv_thermostat_self*)param;
    if(*vv_thermostat_get_actual_value(self) < *vv_thermostat_get_reference_value(self)) {
	vv_thermostat_set_actual_state(self, true);
    } else if(*vv_thermostat_get_actual_value(self) > (1.05 * *vv_thermostat_get_reference_value(self))) {
	vv_thermostat_set_actual_state(self, false);
    }

    bc_scheduler_plan_relative(vv_thermostat.vv_thermostat_task_id, VV_THERMOSTAT_TASK_DELAY);
}
