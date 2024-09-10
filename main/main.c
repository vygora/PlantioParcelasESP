#include <stdio.h>
#include <stdint.h>
#include "slave.h"
#include "mcpwm_helper.h"
#include "pcnt_helper.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"

// static const char *TAG = "main";
static const int MAIN_LOOP_DELAY_TICKS = 1;
// static const int MICROSCONDS_PER_MILISECOND = 1000;

typedef struct 
{
    int16_t input_pulse_count_enabled;
    int16_t input_pulse_count;
    int16_t input_pulse_threshold;
    int16_t output_pulse_enable;
    int16_t trigger_output_pulse;
    int16_t input_pulse_debouncing_time;
    int16_t output_pulse_duration;
}slave_registers_t;

slave_registers_t initialize_default_slave_registers()
{
    return (slave_registers_t) {
        .input_pulse_count_enabled = 0,
        .input_pulse_count = 0,
        .input_pulse_threshold = INT16_MAX,
        .output_pulse_enable = false,
        .trigger_output_pulse = false,
        .input_pulse_debouncing_time = 10,
        .output_pulse_duration = 300, 
    };
}

volatile slave_registers_t mb_registers;
volatile slave_registers_t last_registers;

// static bool IRAM_ATTR input_isr(mcpwm_cap_channel_handle_t cap_channel, const mcpwm_capture_event_data_t *edata, void *user_data)
// {
//     static uint64_t prev_isr_time = 0;

//     uint64_t current_time = esp_timer_get_time();
//     uint64_t delta_time_us = current_time - prev_isr_time;
//     uint64_t debounce_time_microseconds = mb_registers.
//         input_pulse_debouncing_time * MICROSCONDS_PER_MILISECOND;

//     if (delta_time_us >= debounce_time_microseconds)
//     {
//         prev_isr_time = current_time;

//         if (mb_registers.input_pulse_count_enabled)
//         {
//             mb_registers.input_pulse_count++;

//             if(mb_registers.input_pulse_count >= mb_registers.input_pulse_threshold)
//             {
//                 mb_registers.trigger_output_pulse = true;
//                 mb_registers.input_pulse_count = 0;
//             }
//         }                       
//     }
    
//     return true;
// }

void update_task(void* pvParameters) {

    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
        if(mb_registers.input_pulse_count_enabled != last_registers.input_pulse_count_enabled) {
            if(mb_registers.input_pulse_count_enabled)
                pcnt_start_count();
            else
                pcnt_stop_count();
                pcnt_clear_count();
        }

        last_registers.input_pulse_count_enabled = mb_registers.input_pulse_count_enabled;
        last_registers.input_pulse_count = mb_registers.input_pulse_count;
        last_registers.input_pulse_threshold = mb_registers.input_pulse_threshold;
        last_registers.output_pulse_enable = mb_registers.output_pulse_enable;
        last_registers.trigger_output_pulse = mb_registers.trigger_output_pulse;
        last_registers.input_pulse_debouncing_time = mb_registers.input_pulse_debouncing_time;
        last_registers.output_pulse_duration = mb_registers.output_pulse_duration;

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void count_task(void * pvParameters) {
   
   while(1) {
        mb_registers.input_pulse_count += pcnt_get_count();
        pcnt_clear_count();

        if(mb_registers.input_pulse_count >= mb_registers.input_pulse_threshold)
        {
            mb_registers.trigger_output_pulse = true;
            mb_registers.input_pulse_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void pulse_task()
{
    mb_registers.trigger_output_pulse = false;
    if (mb_registers.output_pulse_enable)
    {
        pwm_start();
        vTaskDelay(pdMS_TO_TICKS(mb_registers.output_pulse_duration));
        pwm_stop();
    }
    
    vTaskDelete(NULL);
}

void app_main(void)
{
    TaskHandle_t pulse_task_handle = NULL;
    TaskHandle_t update_task_handle = NULL;
    xTaskCreate(update_task, "update_task", 2048, NULL, 0, &update_task_handle);

    pwm_init();
    // capture_timer_start(input_isr);
    pcnt_init();
    slave_init((uint16_t*)&mb_registers, sizeof(mb_registers), update_task_handle);
    
    mb_registers = initialize_default_slave_registers();

    
    xTaskCreate(count_task, "count_task", 2048, NULL, 0,NULL);

    while(true)
    {
        if (mb_registers.trigger_output_pulse)
            xTaskCreate(pulse_task, "pulse_task", 4096, NULL, 0, &pulse_task_handle);
        
        if (!mb_is_connected())
            mb_registers = initialize_default_slave_registers();
        vTaskDelay(MAIN_LOOP_DELAY_TICKS);
    }
}