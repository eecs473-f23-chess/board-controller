#include "mobile_app_ble.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gatt_common_api.h>
#include <esp_gatt_defs.h>
#include <esp_gattc_api.h>
#include <esp_log.h>
#include <freertos/task.h>

#define MOBILE_APP_BLE_APP_ID       0

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
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static void mobile_app_ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
    static const char* TAG = "MOBILE_APP_BLE_GA_EVENT_HANDLER";

    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "EESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT");
            break;
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
        break;
        default:
            ESP_LOGI(TAG, "Unahandled event %d", event);
            break;
    }
}

static void mobile_app_ble_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    static const char* TAG = "MOBILE_APP_BLE_GATTC_CALLBACK";

    switch (event) {
        case ESP_GATTC_REG_EVT:
            ESP_LOGI(TAG, "ESP_GATTC_REG_EVT");

            // Set device name
            esp_err_t ret = esp_ble_gap_set_device_name("ESP32-S3");
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
        default:
            ESP_LOGI(TAG, "Unhandled event %d", event);
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