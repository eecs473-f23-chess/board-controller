idf_component_register(SRCS "board_controller.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash lichess_api user_config)
