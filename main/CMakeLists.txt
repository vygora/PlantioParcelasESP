set(req driver freertos log esp_common esp_timer)

idf_component_register(SRCS "slave/slave.c" "main.c" "mcpwm/mcpwm_helper.c" "pcnt/pcnt_helper.c"
                    INCLUDE_DIRS "." "mcpwm" "slave" "pcnt"
                    REQUIRES ${req})