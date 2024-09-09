#include <stdio.h>
#include "mcpwm_helper.h"
#include <esp_err.h>

#define TIMER_RESOLUTION_HZ 1000000 // 1Mhz
#define MCPWM_PERIOD 1000 // 1ms

// #define TOP_SWITCH 17
// #define BOTTOM_SWITCH 2
// #define SENSOR_INPUT 32

// Saída 1
#define TOP_SWITCH 33
#define BOTTOM_SWITCH 25
// Sensor 1
#define SENSOR_INPUT 16
#define COMPARE_VALUE 900

static const char *TAG = "mcpwm_helper";
static mcpwm_timer_handle_t timer = NULL;
static mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = TIMER_RESOLUTION_HZ,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks = MCPWM_PERIOD,
};

static mcpwm_oper_handle_t operator = NULL;
static mcpwm_operator_config_t operator_config = {
    .group_id = 0,
};

static mcpwm_cmpr_handle_t comparator = NULL;
static mcpwm_comparator_config_t comparator_config = {
    .flags.update_cmp_on_tez = true,
};

static mcpwm_gen_handle_t generators[2] = {};
static mcpwm_generator_config_t gen_config = {};

void pwm_generator_set_actions(mcpwm_gen_handle_t generator)
{
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(
        generator, 
        MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH), 
        MCPWM_GEN_TIMER_EVENT_ACTION_END()
    ));
    
    ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(
        generator,
        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW),
        MCPWM_GEN_COMPARE_EVENT_ACTION_END()
    ));
}

void pwm_init()
{
    ESP_LOGI(TAG, "Create MCPWM timer");
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    ESP_LOGI(TAG, "Create MCPWM operator");
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &operator));

    ESP_LOGI(TAG, "Connect operator to the timer");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator, timer));

    ESP_LOGI(TAG, "Create comparators");
    ESP_ERROR_CHECK(mcpwm_new_comparator(operator, &comparator_config, &comparator));

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, COMPARE_VALUE));

    ESP_LOGI(TAG, "Create PWM generators");
    gen_config.gen_gpio_num = TOP_SWITCH;
    ESP_ERROR_CHECK(mcpwm_new_generator(operator, &gen_config, &generators[0]));
    gen_config.gen_gpio_num = BOTTOM_SWITCH;
    ESP_ERROR_CHECK(mcpwm_new_generator(operator, &gen_config, &generators[1]));
    
    ESP_LOGI(TAG, "Set generator actions");
    pwm_generator_set_actions(generators[0]);
    pwm_generator_set_actions(generators[1]);

    ESP_LOGI(TAG, "Setup deadtime");
    mcpwm_dead_time_config_t dead_time_config = {
        .posedge_delay_ticks = 5,
        .negedge_delay_ticks = 0
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generators[0], generators[0], &dead_time_config));

    dead_time_config = (mcpwm_dead_time_config_t) {
        .negedge_delay_ticks = 5,
        .flags.invert_output = true,
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generators[1], generators[1], &dead_time_config));

    mcpwm_generator_set_force_level(generators[0], 0, true);
    // because gen_low is inverted by dead time module, so we need to set force level to 1
    mcpwm_generator_set_force_level(generators[1], 0, true);

    ESP_LOGI(TAG, "Start the MCPWM timer");
    mcpwm_timer_enable(timer);
    mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);
}

void pwm_start()
{
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generators[0], -1, true));
    ESP_ERROR_CHECK(mcpwm_generator_set_force_level(generators[1], -1, true));
}

void pwm_stop()
{
    mcpwm_generator_set_force_level(generators[0], 0, true);
    // because gen_low is inverted by dead time module, so we need to set force level to 0
    mcpwm_generator_set_force_level(generators[1], 0, true);
}

void capture_timer_start(mcpwm_capture_event_cb_t callback)
{   
    mcpwm_cap_timer_handle_t cap_timer = NULL;
    mcpwm_capture_timer_config_t cap_timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_timer_config, &cap_timer));

    mcpwm_cap_channel_handle_t cap_channel = NULL;
    mcpwm_capture_channel_config_t cap_channel_config = {
        .prescale = 1,
        .flags.pull_up = true,
        .flags.pos_edge = true,
        .gpio_num = SENSOR_INPUT,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &cap_channel_config, &cap_channel));

    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = callback
    };

    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_channel, &cbs, NULL));
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_channel));

    ESP_LOGI(TAG, "Enable and start capture timer");
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));
}
