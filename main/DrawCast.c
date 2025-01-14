#include <stdio.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "esp_log.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

#define DISPLAY_HEIGHT 10
#define DISPLAY_WIDTH 10

char draw_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
int cursor_x;
int cursor_y;
bool drawing;

inline int min(int a, int b) {
    return a > b ? b : a;
}

inline int max(int a, int b) {
    return a < b ? b : a;
}

void display_frame_buffer() {
    char buffer_string[DISPLAY_HEIGHT * (DISPLAY_WIDTH + 1) + 1];
    int idx = 0;
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            buffer_string[idx] = draw_buffer[y][x] + 0x30;
            idx++;
        }
        buffer_string[idx] = '\n';
        idx++;
    }
    buffer_string[idx] = '\0';
    ESP_LOGI("FRAMEBUFFER", "\n%s", buffer_string);
}

void handle_input(char input) {
    if (input == ' ') {
        drawing = !drawing;
        if (drawing) {
            draw_buffer[cursor_y][cursor_x] = 1;
        }
        ESP_LOGI("DIRECTION", "Drawing flag set to: %d", drawing);
        return;
    }

    if (input == 'w') {
        ESP_LOGI("DIRECTION", "Up");
        cursor_y = max(0, cursor_y - 1);
    } else if (input == 'a') {
        ESP_LOGI("DIRECTION", "Left");
        cursor_x = max(0, cursor_x - 1);
    } else if (input == 's') {
        ESP_LOGI("DIRECTION", "Down");
        cursor_y = min(DISPLAY_HEIGHT - 1, cursor_y + 1);
    } else if (input == 'd') {
        ESP_LOGI("DIRECTION", "Right");
        cursor_x = min(DISPLAY_WIDTH - 1, cursor_x + 1);
    }

    ESP_LOGI("DIRECTION", "New coords: %d, %d", cursor_y, cursor_x);
    
    if (!drawing) { return; }
    draw_buffer[cursor_y][cursor_x] = 1;
    display_frame_buffer();
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
            handle_input(data[0]);
        }
    }
}
