#include "pcnt_helper.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

#define EXAMPLE_PCNT_HIGH_LIMIT 10000
#define EXAMPLE_PCNT_LOW_LIMIT  -10000

// Sensor 1
#define SENSOR_INPUT 16

static const char *TAG = "pcnt_helper";
static pcnt_unit_handle_t pcnt_unit = NULL;

void pcnt_init() {
    ESP_LOGI(TAG, "Install PCNT unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    ESP_LOGI(TAG, "Set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    ESP_LOGI(TAG, "Install PCNT channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = SENSOR_INPUT,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    ESP_LOGI(TAG, "Set edge and level actions for PCNT channels");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_HOLD, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP));

    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

int pcnt_get_count() {
    int pulse_count = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
    return pulse_count;
}

void pcnt_clear_count() {
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
}

void pcnt_stop_count() {
    ESP_ERROR_CHECK(pcnt_unit_stop(pcnt_unit));
}

void pcnt_start_count() {
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}