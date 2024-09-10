#include "slave.h"

#include <stdio.h>
#include "esp_err.h"
#include "mbcontroller.h"       // for mbcontroller defines and api
//#include "modbus_params.h"      // for modbus parameters structures
#include "esp_log.h"            // for log_write
#include "sdkconfig.h"
#include "esp_timer.h"

static const char *TAG = "modbus-slave";

#define MB_SLAVE_DEV_SPEED 115200
#define MB_SLAVE_ADDRESS 1
#define MB_SLAVE_PORT_NUM 2
#define MB_SLAVE_TX_PIN 18//17 // 1
#define MB_SLAVE_RX_PIN 17//16 // 3
#define MB_SLAVE_RTS_PIN 5//12

#define MB_READ_MASK                        (MB_EVENT_INPUT_REG_RD \
                                                | MB_EVENT_HOLDING_REG_RD \
                                                | MB_EVENT_DISCRETE_RD \
                                                | MB_EVENT_COILS_RD)

#define MB_WRITE_MASK                       (MB_EVENT_HOLDING_REG_WR \
                                                | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK                  (MB_READ_MASK | MB_WRITE_MASK)

//static portMUX_TYPE param_lock = portMUX_INITIALIZER_UNLOCKED;

esp_err_t setup_communication()
{
    mb_communication_info_t comm_info = {
        .mode = MB_MODE_RTU,
        .slave_addr = MB_SLAVE_ADDRESS,
        .port = MB_SLAVE_PORT_NUM,
        .baudrate = MB_DEVICE_SPEED,
        .parity = MB_PARITY_NONE,
    };

    return mbc_slave_setup((void*)&comm_info);
}

static uint64_t last_message_time = 0;

uint64_t get_last_message_time()
{
    return last_message_time;
}

bool mb_is_connected()
{
    uint32_t timeout = 10e6; // 10 s
    uint64_t curr = esp_timer_get_time();
    return ((last_message_time + timeout) > curr);
}
void event_task(void* taskHandle)
{
    while (1)
    {
        mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);

        if(event & (MB_EVENT_HOLDING_REG_WR))
            xTaskNotifyGive(taskHandle);

        last_message_time = esp_timer_get_time();
        //ESP_LOGI(TAG, "Recebido: %llu", last_message_time);
    }
    vTaskDelete(NULL);
}
void slave_init(uint16_t *registers, uint16_t registers_length, TaskHandle_t task_handle)
{
    void *slave_handler = NULL;
    ESP_LOGI(TAG, "Slave init");
    ESP_ERROR_CHECK(mbc_slave_init(MB_PORT_SERIAL_SLAVE, &slave_handler));

    ESP_LOGI(TAG, "Set descriptor");
    mb_register_area_descriptor_t reg_descriptor = {
        .type = MB_PARAM_HOLDING,
        .address = registers,
        .size = registers_length,
    };
    ESP_ERROR_CHECK(mbc_slave_set_descriptor(reg_descriptor));

    ESP_LOGI(TAG, "Slave setup");
    ESP_ERROR_CHECK(setup_communication());

    ESP_LOGI(TAG, "Slave start");
    ESP_ERROR_CHECK(mbc_slave_start());

    ESP_ERROR_CHECK(uart_set_pin(MB_SLAVE_PORT_NUM, MB_SLAVE_TX_PIN,
                                MB_SLAVE_RX_PIN, MB_SLAVE_RTS_PIN,
                                UART_PIN_NO_CHANGE));

    // // Set UART driver mode to Half Duplex
    ESP_ERROR_CHECK(uart_set_mode(MB_SLAVE_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX));

    ESP_LOGI(TAG, "Modbus slave stack initialized.");

    TaskHandle_t handle = NULL;
    xTaskCreate(event_task, "event_task", 4096, (void*) task_handle, 0, &handle);
}
