#ifndef __MCPWM_HELPER_H__
#define __MCPWM_HELPER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"

void pwm_init();
void pwm_start();
void pwm_stop();
void capture_timer_start(mcpwm_capture_event_cb_t callback);

#endif // !__MCPWM_HELPER_H__