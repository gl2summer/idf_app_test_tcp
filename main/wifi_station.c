/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "sta";


void smartconfig_evt_cb(SC_EVENT evt, void *p_data){

	switch(evt){

	case SC_EVT_LINK:
		wifi_config_t *wifi_config = p_data;
        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_connect() );
		break;

	case SC_EVT_LINK_OVER:
        xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
		break;

	default:
		break;
	}
}


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	wifi_config_t config;

	ESP_LOGI(TAG, "event recv:%d\n",event->event_id);

    switch(event->event_id) {

    case SYSTEM_EVENT_STA_START:
    	ESP_ERROR_CHECK( esp_wifi_get_config (ESP_IF_WIFI_STA, &config) );
    	ESP_LOGI(TAG, "wifi name: %s, password: %s", config.sta.ssid, config.sta.password);

    	if((strlen((char *)config.sta.ssid)==0)||(strlen((char *)config.sta.password)==0)){
    		//若获取wifi配置失败，则打开smartconfig
			smartconfig_start(smartconfig_evt_cb);
			xTaskCreate(smartconfig_monitor_task, "smartconfig_monitor_task", 4096, NULL, 3, NULL);
		}
    	else{
    		ESP_ERROR_CHECK( esp_wifi_connect() );//若获取wifi配置成功，则连接
    	}
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;

    default:
        break;
    }
    return ESP_OK;
}

void smartconfig_monitor_task(void * parm)
{
    EventBits_t uxBits;
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits==0){
        	ESP_LOGI(TAG, "smartconfig timeout, restart");
        	smartconfig_restart();
        	continue;
        }
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

static void initialise_wifi(void)
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    ESP_ERROR_CHECK( esp_wifi_start() );
}

void wifi_station_main()
{
    initialise_wifi();
}

