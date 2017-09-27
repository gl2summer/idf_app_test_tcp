#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "smartconfig.h"
#include "esp_wifi.h"

#define TAG "app"

void app_main()
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

	smartconfig_main();
}

