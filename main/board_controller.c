#include <nvs_flash.h>
#include <stddef.h>

#include "mobile_app_ble.h"

void app_main(void)
{
    nvs_flash_init();
    mobile_app_ble_init();
}
