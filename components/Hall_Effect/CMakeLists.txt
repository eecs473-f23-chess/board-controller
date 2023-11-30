idf_component_register(SRCS "Hall_Effect.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver board_state types freertos esp_adc hal)
