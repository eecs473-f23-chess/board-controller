#include "wifi.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include <string.h>

#define WIFI_MAX_RETRY      5
#define WIFI_CONNECT_BIT    BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_START_BIT

static char ssid[MAX_SSID_LEN];
static char pw[MAX_PASSPHRASE_LEN];

static EventGroupHandle_t wifi_event_group;

static bool is_connected;
static bool ssid_set;
static bool pw_set;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    const char* TAG = "WIFI_EVENT_HANDLER";
    static int retry_num = 0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            retry_num = 0;
            is_connected = false;
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        is_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECT_BIT);
    }
}

void wifi_init() {
    is_connected = false;
    ssid_set = false;
    pw_set = false;
    // char *ssid_replace = "Braeden's Galaxy S22 Ultra";
    // char *pwd_replace = "sefk6040";
    
    char *ssid_replace = "AiPhone";
    char *pwd_replace = "password";

    printf("Wifi: %s | Pass: %s\n", ssid_replace, pwd_replace);

    // char *ssid_replace = "Aditya's iPhone (3)";
    // char *pwd_replace = "testingone";

    for(int i = 0; i < strlen(ssid_replace); i++){
        ssid[i] = ssid_replace[i];
    }
    for(int i = 0; i < strlen(pwd_replace); i++){
        pw[i] = pwd_replace[i];
    }
    // ssid[0] = 0;
    // pw[0] = 0;

    wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
}

bool wifi_is_connected() {
    return is_connected;
}

char* wifi_get_ssid() {
    return ssid;
}

void wifi_set_ssid(const char* ssid_buf, const uint16_t ssid_len) {
    static const char* TAG = "WIFI_SET_SSID";
    if ((ssid_len + 1) > MAX_SSID_LEN) {
        ESP_LOGW(TAG, "Wifi SSID too long");
        return;
    }

    memcpy(ssid, ssid_buf, ssid_len);
    ssid[ssid_len] = 0;
}

void wifi_set_pw(const char* pw_buf, const uint16_t pw_len) {
    static const char* TAG = "WIFI_SET_PW";
    if ((pw_len + 1) > MAX_PASSPHRASE_LEN) {
        ESP_LOGW(TAG, "Wifi pw too long");
        return;
    }
    char *temp = "sefk6040";
    for(int i = 0; i < strlen(temp); i++){
        pw[i] = temp[i];
    }
    printf("Password %s\n", pw);
    // memcpy(pw, pw_buf, pw_len);
    // pw[pw_len] = 0;
}

bool wifi_connect() {
    const char* TAG = "CONNECT_WIFI";

    wifi_event_group = xEventGroupCreate();

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char*)wifi_config.sta.ssid, ssid);
    strcpy((char*)wifi_config.sta.password, pw);

    esp_wifi_stop();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished");

    // Blocks until wifi status received
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECT_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECT_BIT) {
        ESP_LOGI(TAG, "Wifi connected");
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "Failed to connect to wifi");
        return false;
    } else {
        ESP_LOGE(TAG, "Unexpected event");
        return false;
    }
}
