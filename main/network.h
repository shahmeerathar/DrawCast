#include "secret.h"

#ifdef USER_A_CREDS
#define MQTT_PUB_TOPIC "drawcast/a"
#define MQTT_SUB_TOPIC "drawcast/b"
#else
#define MQTT_PUB_TOPIC "drawcast/b"
#define MQTT_SUB_TOPIC "drawcast/a"
#endif

void wifi_connect();

typedef void (*mqtt_message_callback_t)(char* data, void* context);
void mqtt_connect(mqtt_message_callback_t cb, void* context);
void mqtt_send_message(char* data);
