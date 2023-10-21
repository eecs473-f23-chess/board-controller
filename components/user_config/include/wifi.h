#include <stdbool.h>
#include <stdint.h>

void wifi_init();
bool wifi_is_connected();
char* wifi_get_ssid();
void wifi_set_ssid(const char* ssid_buf, const uint16_t ssid_len);
void wifi_set_pw(const char* pw_buf, const uint16_t pw_len);
bool wifi_connect();
