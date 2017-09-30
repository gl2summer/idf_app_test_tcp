#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "smartconfig.h"
#include "wifi_station.h"
#include "driver/gpio.h"

#define TAG "app"


#define RESET_WIFI_GPIO CONFIG_RESET_WIFI_GPIO











void app_main()
{
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    int delay = 10;
    gpio_pad_select_gpio(RESET_WIFI_GPIO);
    gpio_set_pull_mode(RESET_WIFI_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_direction(RESET_WIFI_GPIO, GPIO_MODE_INPUT);
    while((gpio_get_level(RESET_WIFI_GPIO)==0)&&(--delay)){
    	ets_delay_us(10000);
    }

    wifi_station_main(delay==0);
}

