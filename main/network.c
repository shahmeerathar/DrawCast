#include "network.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

static esp_mqtt_client_handle_t mqtt_client = NULL;
static mqtt_message_callback_t mqtt_message_callback = NULL;
static void* mqtt_message_callback_context = NULL;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    }
}

void wifi_connect()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI("MQTT", "MQTT Connected");
        esp_mqtt_client_subscribe(event->client, MQTT_SUB_TOPIC, 1);
        break;
    case MQTT_EVENT_DATA:
        mqtt_message_callback(event->data, mqtt_message_callback_context);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI("MQTT", "MQTT Disconnected");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI("MQTT", "MQTT Error");
        break;
    default:
        break;
    }
}

void mqtt_connect(mqtt_message_callback_t cb, void* cb_context)
{
    mqtt_message_callback = cb;
    mqtt_message_callback_context = cb_context;

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = AWS_DOMAIN,
            .verification.certificate = AWS_ROOTCA1,
        },
        .credentials = {
            .authentication = {
                .certificate = AWS_CERT,
                .key = AWS_PRIVATE_KEY,
            },
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}

void mqtt_send_message(char* data)
{
    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_PUB_TOPIC, data, 0, 1, 0);
    ESP_LOGI("MQTT", "Published message ID: %d", msg_id);
}
