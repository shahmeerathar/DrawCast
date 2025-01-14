#include <stdio.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "esp_log.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

#define DISPLAY_HEIGHT 128
#define DISPLAY_WIDTH 128

char draw_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
int cursor_x;
int cursor_y;
bool drawing;

void handle_direction(char dir) {
    if (dir == 'w') {
        ESP_LOGI("DIRECTION", "Up");
    } else if (dir == 'a') {
        ESP_LOGI("DIRECTION", "Left");
    } else if (dir == 's') {
        ESP_LOGI("DIRECTION", "Down");
    } else if (dir == 'd') {
        ESP_LOGI("DIRECTION", "Right");
    }
}

void app_main(void)
{
    cursor_x = 0;
    cursor_y = 0;
    drawing = false;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_HEIGHT; x++) {
            draw_buffer[y][x] = 0;
        }
    }

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);

    uint8_t data[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len == 1) {
            handle_direction(data[0]);
        }
    }
}
