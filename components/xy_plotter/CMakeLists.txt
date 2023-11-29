idf_component_register(SRCS "xy_plotter.c"
                    INCLUDE_DIRS "include"
                    REQUIRES board_state driver freertos lichess_api)
