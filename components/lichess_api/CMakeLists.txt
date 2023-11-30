idf_component_register(SRCS "lichess_api.c"
                    INCLUDE_DIRS "include"
                    REQUIRES xy_plotter esp_http_client esp_wifi nvs_flash json driver freertos user_config Buttons Hall_Effect types board_state Buttons)
