#include "network.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "secret.h"

void wifi_connect()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    vTaskDelay(pdMS_TO_TICKS(3000));
    vTaskDelay(pdMS_TO_TICKS(1000));
}
}
