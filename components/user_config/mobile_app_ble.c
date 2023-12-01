#include "mobile_app_ble.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gatt_common_api.h>
#include <esp_gatt_defs.h>
#include <esp_gattc_api.h>
#include <esp_log.h>
#include <freertos/task.h>
#include <inttypes.h>
#include <string.h>

#include "Buttons.h"
#include "lichess_api.h"
#include "wifi.h"

#define MOBILE_APP_BLE_APP_ID   0

// Wifi service
#define WIFI_SERVICE_UUID           0x01
#define SSID_CHAR_UUID              0x01
#define PW_CHAR_UUID                0x02
#define CONNECTED_UUID              0x03

// Lichess service
#define LICHESS_SERVICE_UUID        0x02
#define BEARER_TOKEN_CHAR_UUID      0x01

// Game config service
#define GAME_SERVICE_UUID           0x03
#define TIME_CONTROL_CHAR_UUID      0x01
#define OPPONENT_TYPE_CHAR_UUID     0x02
#define OPPONENT_USERNAME_CHAR_UUID 0x03
#define BUTTON_CHAR_UUID            0x04

// Characteristic handles
static uint16_t wifi_ssid_char_handle;
static uint16_t wifi_pw_char_handle;
static uint16_t wifi_connected_char_handle;
static uint16_t lichess_bearer_token_char_handle;
static uint16_t time_control_char_handle;
static uint16_t opponent_type_char_handle;
static uint16_t opponent_username_char_handle;
static uint16_t button_char_handle;

static uint8_t adv_manufacturer_data[] = {0xFF, 0xFF, 0x41, 0x41, 0x42, 0x4D, 0x52};

static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t ble_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = sizeof(adv_manufacturer_data),
    .p_manufacturer_data = adv_manufacturer_data,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_gatt_if_t ble_gatt_if;
uint16_t ble_conn_id;
static esp_bd_addr_t ble_remote_addr;

static void mobile_app_ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    static const char* TAG = "MOBILE_APP_BLE_GAP_EVENT_HANDLER";

    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT");

            esp_err_t ret = esp_ble_gap_start_advertising(&ble_adv_params);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed starting advertising: %s", esp_err_to_name(ret));
                exit(1);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            ESP_LOGI(TAG, "ESP_GAP_BLE_ADV_START_COMPLETE_EVT");
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed\n");
            }
            break;
        default:
            ESP_LOGI(TAG, "Unahandled event %d", event);
            break;
    }
}

static void mobile_app_ble_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    static const char* TAG = "MOBILE_APP_BLE_GATTC_CALLBACK";
    esp_err_t ret;
    esp_gatt_status_t gatt_ret;

    switch (event) {
        case ESP_GATTC_REG_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_REG_EVT");

            // Set device name
            ret = esp_ble_gap_set_device_name("ESP32-S3");
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed Setting device name: %s", esp_err_to_name(ret));
                exit(1);
            }

            // Configure advertising data
            ret = esp_ble_gap_config_adv_data(&ble_adv_data);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed configuring advertising data: %s", esp_err_to_name(ret));
                exit(1);
            }
            break;
        case ESP_GATTC_CONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_CONNECT_EVT");

            // Store connection parameters
            ble_gatt_if = gattc_if;
            ble_conn_id = param->connect.conn_id;
            memcpy(ble_remote_addr, param->connect.remote_bda, sizeof(ble_remote_addr));

            // Open connection
            ret = esp_ble_gattc_open(ble_gatt_if, ble_remote_addr, param->connect.ble_addr_type, true);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed opening gattc connection: %s", esp_err_to_name(ret));
                exit(1);
            }
            break;
        case ESP_GATTC_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_DISCONNECT_EVT");
            break;
        case ESP_GATTC_OPEN_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_OPEN_EVT");

            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (ble_gatt_if, ble_conn_id);
            if (mtu_ret != ESP_OK) {
                ESP_LOGE(TAG, "config MTU error, error code = %x", mtu_ret);
            }
            break;
        case ESP_GATTC_CLOSE_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_CLOSE_EVT");
            break;
        case ESP_GATTC_CFG_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_CFG_MTU_EVT");
            if (param->cfg_mtu.status != ESP_GATT_OK){
                ESP_LOGE(TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
            }
            break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_DIS_SRVC_CMPL_EVT");
            if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
                exit(1);
            }

            // Search for services
            gatt_ret = esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, NULL);
            if (gatt_ret != ESP_GATT_OK) {
                ESP_LOGE(TAG, "Failed starting service search");
                exit(1);
            }
            break;
        case ESP_GATTC_SEARCH_RES_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_SEARCH_RES_EVT");
            struct gattc_search_res_evt_param* search_res = &param->search_res;
            uint16_t service_uuid = search_res->srvc_id.uuid.uuid.uuid16;

            uint16_t expected_num_chars;
            switch (service_uuid) {
                case WIFI_SERVICE_UUID:
                    ESP_LOGI(TAG, "Found wifi service");
                    expected_num_chars = 3;
                    break;
                case LICHESS_SERVICE_UUID:
                    ESP_LOGI(TAG, "Found lichess service");
                    expected_num_chars = 1;
                    break;
                case GAME_SERVICE_UUID:
                    ESP_LOGI(TAG, "Found game service");
                    expected_num_chars = 4;
                    break;
                default:
                    ESP_LOGW(TAG, "Received unhandled service UUID %" PRIu16, service_uuid);
                    return;
            }

            // Get all characteristics corresponding to this service
            esp_gattc_char_elem_t* chars = malloc(expected_num_chars * sizeof(esp_gattc_char_elem_t));
            uint16_t num_chars = expected_num_chars;
            gatt_ret = esp_ble_gattc_get_all_char(ble_gatt_if, ble_conn_id, search_res->start_handle, search_res->end_handle, chars, &num_chars, 0);
            if (gatt_ret != ESP_GATT_OK) {
                ESP_LOGE(TAG, "Failed getting service characteristics");
                exit(1);
            }
            if (num_chars != expected_num_chars) {
                ESP_LOGE(TAG, "Expected to receive %" PRIu16 " characteristics, actually received %" PRIu16, expected_num_chars, num_chars);
                exit(1);
            }

            // Iterate through all characteristics found
            for (uint16_t i = 0; i < num_chars; ++i) {
                uint16_t char_uuid = chars[i].uuid.uuid.uuid16;
                uint16_t char_handle = chars[i].char_handle;
                ret = esp_ble_gattc_register_for_notify(ble_gatt_if, ble_remote_addr, char_handle);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to register characteristic notification");
                    continue;
                }

                switch (service_uuid) {
                    case WIFI_SERVICE_UUID:
                        switch (char_uuid) {
                            case SSID_CHAR_UUID:
                                ESP_LOGI(TAG, "Found SSID characteristic");
                                wifi_ssid_char_handle = char_handle;
                                break;
                            case PW_CHAR_UUID:
                                ESP_LOGI(TAG, "Found password characteristic");
                                wifi_pw_char_handle = char_handle;
                                break;
                            case CONNECTED_UUID:
                                ESP_LOGI(TAG, "Found connected characteristic");
                                wifi_connected_char_handle = char_handle;
                                break;
                            default:
                                ESP_LOGW(TAG, "Unexpected characteristic UUID %" PRIu16, char_uuid);
                                break;
                        }
                        break;
                    case LICHESS_SERVICE_UUID:
                        switch (char_uuid) {
                            case BEARER_TOKEN_CHAR_UUID:
                                ESP_LOGI(TAG, "Found bearer token characteristic");
                                lichess_bearer_token_char_handle = char_handle;
                                break;
                            default:
                                ESP_LOGW(TAG, "Unexpected characteristic UUID %" PRIu16, char_uuid);
                                break;
                        }
                        break;
                    case GAME_SERVICE_UUID:
                        switch (char_uuid) {
                            case TIME_CONTROL_CHAR_UUID:
                                ESP_LOGI(TAG, "Found time control characteristic");
                                time_control_char_handle = char_handle;
                                break;
                            case OPPONENT_TYPE_CHAR_UUID:
                                ESP_LOGI(TAG, "Found opponent type characteristic");
                                opponent_type_char_handle = char_handle;
                                break;
                            case OPPONENT_USERNAME_CHAR_UUID:
                                ESP_LOGI(TAG, "Found opponent username characteristic");
                                opponent_username_char_handle = char_handle;
                                break;
                            case BUTTON_CHAR_UUID:
                                ESP_LOGI(TAG, "Found button characteristic");
                                button_char_handle = char_handle;
                                break;
                            default:
                                ESP_LOGW(TAG, "Unexpected characteristic UUID %" PRIu16, char_uuid);
                                break;
                        }
                        break;
                    default:
                        ESP_LOGE(TAG, "Received unhandled service UUID %" PRIu16, service_uuid);
                        break;
                }
            }

            free(chars);
            break;
        case ESP_GATTC_REG_FOR_NOTIFY_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
            break;
        case ESP_GATTC_SEARCH_CMPL_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_SEARCH_CMPL_EVT");

            // Send wifi ssid
            char* ssid = wifi_get_ssid();
            printf("wifi len: %d\n", strlen(ssid));
            ret = esp_ble_gattc_write_char(ble_gatt_if, ble_conn_id, wifi_ssid_char_handle, strlen(ssid) + 1, (uint8_t*)ssid, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed writing wifi ssid");
                exit(1);
            }

            // Send wifi connection status
            bool is_connected = wifi_is_connected();
            ret = esp_ble_gattc_write_char(ble_gatt_if, ble_conn_id, wifi_connected_char_handle, sizeof(is_connected), (uint8_t*)&is_connected, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed writing wifi connected status");
                exit(1);
            }

            // Send time control
            time_control_t time_control;
            lichess_api_get_time_control(&time_control);
            printf("sending time control %"PRIu8"\n", (uint8_t)(time_control));
            ret = esp_ble_gattc_write_char(ble_gatt_if, ble_conn_id, time_control_char_handle, 1, (uint8_t*)(&time_control), ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed writing time control");
                exit(1);
            }

            // Send opponent type
            opponent_type_t opponent_type;
            lichess_api_get_opponent_type(&opponent_type);
            printf("sending opponent type %"PRIu8"\n", (uint8_t)(opponent_type));
            ret = esp_ble_gattc_write_char(ble_gatt_if, ble_conn_id, opponent_type_char_handle, 1, (uint8_t*)(&opponent_type), ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed writing opponent type");
                exit(1);
            }

            // Send opponent username
            char* opponent_username;
            lichess_api_get_specific_username(&opponent_username);
            uint16_t username_len;
            if (!(*opponent_username)) {
                username_len = 1;
            }
            else {
                username_len = strlen(opponent_username);
            }

            printf("opponent username len: %d\n", strlen(opponent_username));
            ret = esp_ble_gattc_write_char(ble_gatt_if, ble_conn_id, opponent_username_char_handle, username_len, (uint8_t*)(opponent_username), ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed writing opponent type");
                exit(1);
            }

            break;
        case ESP_GATTC_NOTIFY_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT");
            uint16_t data_len = param->notify.value_len;
            uint8_t* data = param->notify.value;
            uint16_t handle = param->notify.handle;

            if (handle == wifi_ssid_char_handle) {
                ESP_LOGI(TAG, "Received wifi ssid");
                wifi_set_ssid((char*)data, data_len);
            }
            else if (handle == wifi_pw_char_handle) {
                ESP_LOGI(TAG, "Received wifi pw");
                wifi_set_pw((char*)data, data_len);
                bool is_connected = wifi_connect();

                // Reply with connection status
                ESP_LOGI(TAG, "Sending connection status");
                ret = esp_ble_gattc_write_char(ble_gatt_if, ble_conn_id, wifi_connected_char_handle, sizeof(is_connected), (uint8_t*)&is_connected, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed writing wifi connected status");
                    exit(1);
                }
            }
            else if (handle == wifi_connected_char_handle) {
                ESP_LOGE(TAG, "Wifi connected characteristic is write only");
            }
            else if (handle == lichess_bearer_token_char_handle) {
                ESP_LOGI(TAG, "Received lichess bearer token");
                lichess_api_login((char*)data, data_len);
            }
            else if (handle == time_control_char_handle) {
                ESP_LOGI(TAG, "Received time control %"PRIu8, *data);
                lichess_api_set_time_control((time_control_t)(*data));
            }
            else if (handle == opponent_type_char_handle) {
                ESP_LOGI(TAG, "Received opponent type %"PRIu8, *data);
                lichess_api_set_opponent_type((opponent_type_t)(*data));
            }
            else if (handle == opponent_username_char_handle) {
                ESP_LOGI(TAG, "Received opponent username %.*s", data_len, data);
                lichess_api_set_specific_username((char*)(data), data_len);
            }
            else if (handle == button_char_handle) {
                int button = *data;
                printf("Received button %d\n", button);
                switch (button) {
                    case 1:
                        make_game_button(NULL);
                        break;
                    case 2:
                        clock_button(NULL);
                        break;
                    case 3:
                        draw_button(NULL);
                        break;
                    case 4:
                        resign_button(NULL);
                        break;
                    default:
                        ESP_LOGE(TAG, "Unhandled button type");
                }
            }
            else {
                ESP_LOGW(TAG, "Data from unhandled characteristic handle received");
            }

            break;
        case ESP_GATTC_WRITE_CHAR_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_WRITE_CHAR_EVT");
            break;

        default:
            ESP_LOGW(TAG, "Unhandled event %d", event);
    }
}

void mobile_app_ble_init(void) {
    static const char* TAG = "MOBILE_APP_BLE_TASK";

    esp_bt_controller_config_t bt_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_bt_controller_init(&bt_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed configuring bluetooth controller: %s", esp_err_to_name(ret));
        exit(1);
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed enabling bluetooth controller: %s", esp_err_to_name(ret));
        exit(1);
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed initializing bluedroid: %s", esp_err_to_name(ret));
        exit(1);
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed enabling bluedroid: %s", esp_err_to_name(ret));
        exit(1);
    }

    ret = esp_ble_gap_register_callback(mobile_app_ble_gap_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed registering gap callback: %s", esp_err_to_name(ret));
        exit(1);
    }

    ret = esp_ble_gattc_register_callback(mobile_app_ble_gattc_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed registering gatt client callback: %s", esp_err_to_name(ret));
        exit(1);
    }

    ret = esp_ble_gattc_app_register(MOBILE_APP_BLE_APP_ID);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed registering gatt client app: %s", esp_err_to_name(ret));
        exit(1);
    }

    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed setting gatt MTU: %s", esp_err_to_name(ret));
        exit(1);
    }
}