#include "mobile_app_ble.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gatt_common_api.h>
#include <esp_gatt_defs.h>
#include <esp_gattc_api.h>
#include <esp_log.h>
#include <freertos/task.h>
#include <string.h>

#define MOBILE_APP_BLE_APP_ID       0

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

            printf("Device addr 2: ");
            for (int i = 0; i < 6; ++i) {
                printf("%02X ", param->connect.remote_bda[i]);
            }
            printf("\n");

            ble_gatt_if = gattc_if;
            ble_conn_id = param->connect.conn_id;
            memcpy(ble_remote_addr, param->connect.remote_bda, sizeof(ble_remote_addr));
            printf("Device addr 1: ");
            for (int i = 0; i < 6; ++i) {
                printf("%02X ", ble_remote_addr[i]);
            }
            printf("\n");

            ret = esp_ble_gattc_open(ble_gatt_if, ble_remote_addr, param->connect.ble_addr_type, true);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed opening gattc connection: %s", esp_err_to_name(ret));
                exit(1);
            }
            break;
        case ESP_GATTC_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_DISCONNECT_EVT");
            break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_DIS_SRVC_CMPL_EVT");
            if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
                exit(1);
            }

            gatt_ret = esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, NULL);
            ESP_LOGI(TAG, "search service result: %d", gatt_ret);
            break;
        case ESP_GATTC_SEARCH_RES_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_SEARCH_RES_EVT");
            esp_gatt_id_t *srvc_id = &(param->search_res.srvc_id);
            uint16_t len = srvc_id->uuid.len;
            if (len == ESP_UUID_LEN_128) {
                uint8_t* uuid = srvc_id->uuid.uuid.uuid128;
                for (int i = 0; i < ESP_UUID_LEN_128; ++i) {
                    printf("%02X ", uuid[i]);
                }
                printf("\n");

                uint16_t count = 10;
                gatt_ret = esp_ble_gattc_get_attr_count(ble_gatt_if, ble_conn_id, ESP_GATT_DB_CHARACTERISTIC, param->search_res.start_handle, param->search_res.end_handle, 0, &count);
                ESP_LOGI(TAG, "gatt_ret for search result: %d", gatt_ret);
                ESP_LOGI(TAG, "number of characteristics in service: %u", count);

                if (count == 2) {
                    esp_gattc_char_elem_t characteristics[2];

                    gatt_ret = esp_ble_gattc_get_all_char(ble_gatt_if, ble_conn_id, param->search_res.start_handle, param->search_res.end_handle, characteristics, &count, 0);
                    ESP_LOGI(TAG, "this time found %u characteristics", count);

                    for (int i = 0; i < count; ++i) {
                        printf("for characteristic %d\n", i);
                        for (int j = 0; j < ESP_UUID_LEN_128; ++j) {
                            printf("%02X ", characteristics[i].uuid.uuid.uuid128[j]);
                        }
                        printf("\n");

                        esp_ble_gattc_register_for_notify(ble_gatt_if, ble_remote_addr, characteristics[i].char_handle);
                    }
                }
            }
            ESP_LOGI(TAG, "Service UUID len: %u", len);
            break;
        case ESP_GATTC_NOTIFY_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT");
            uint16_t data_len = param->notify.value_len;
            uint8_t* data = param->notify.value;

            printf("Received %u bytes of data", data_len);
            printf("Data: ");
            for (int i = 0; i < data_len; ++i) {
                printf("%02X ", data[i]);
            }
            printf("\n");
            break;
        case ESP_GATTC_OPEN_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_OPEN_EVT");
            ESP_LOGI(TAG, "status: %d", param->open.status);
            printf("Device addr 1: ");
            for (int i = 0; i < 6; ++i) {
                printf("%02X ", param->open.remote_bda[i]);
            }
            printf("\n");

            esp_log_buffer_hex(TAG, ble_remote_addr, sizeof(esp_bd_addr_t));
            esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (ble_gatt_if, ble_conn_id);
            if (mtu_ret){
                ESP_LOGE(TAG, "config MTU error, error code = %x", mtu_ret);
            }
            break;
        case ESP_GATTC_CFG_MTU_EVT:
            if (param->cfg_mtu.status != ESP_GATT_OK){
                ESP_LOGE(TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
            }
            ESP_LOGI(TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
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

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}