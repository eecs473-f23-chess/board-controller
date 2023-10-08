idf_component_register(SRCS "board_controller.c"
                    INCLUDE_DIRS "."
                    REQUIRES clock_display nvs_flash user_config score_display)
