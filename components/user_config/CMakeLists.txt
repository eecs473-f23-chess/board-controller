idf_component_register(SRCS "mobile_app_ble.c" "user_config.c" "wifi.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bt lichess_api Buttons)

