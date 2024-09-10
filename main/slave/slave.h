#ifndef SLAVE_H
#define SLAVE_H
#include <inttypes.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void slave_init(uint16_t *registers, uint16_t registers_length, TaskHandle_t task_handle);
uint64_t get_last_message_time();
bool mb_is_connected();
#endif // !SLAVE_H