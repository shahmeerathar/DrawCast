#include "input.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

#define LEFT_BUTTON_PIN 26
#define RIGHT_BUTTON_PIN 32
#define UP_BUTTON_PIN 33
#define DOWN_BUTTON_PIN 25
#define TOGGLE_DRAWING_BUTTON_PIN 27
#define PUSH_BUTTON_PIN_MASK ((1ULL << RIGHT_BUTTON_PIN) | (1ULL << LEFT_BUTTON_PIN) | (1ULL << UP_BUTTON_PIN) | (1ULL << DOWN_BUTTON_PIN) | (1ULL << TOGGLE_DRAWING_BUTTON_PIN))

void setup_uart_input()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);
}

void setup_push_button_input()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = PUSH_BUTTON_PIN_MASK;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_set_direction(RIGHT_BUTTON_PIN, GPIO_MODE_INPUT);
}

Input handle_uart_input()
{
    uint8_t data[BUF_SIZE];
    int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
    if (len != 1) {
        return NONE;
    }

    char input = data[0];
    if (input == ' ') {
        return TOGGLE_DRAWING;
    } else if (input == 'w') {
        return UP;
    } else if (input == 'a') {
        return LEFT;
    } else if (input == 's') {
        return DOWN;
    } else if (input == 'd') {
        return RIGHT;
    }

    return NONE;
}

Input handle_push_button_input()
{
    int right_button_output = gpio_get_level(RIGHT_BUTTON_PIN);
    if (right_button_output == 1) {
        return RIGHT;
    }
    int left_button_output = gpio_get_level(LEFT_BUTTON_PIN);
    if (left_button_output == 1) {
        return LEFT;
    }
    int up_button_output = gpio_get_level(UP_BUTTON_PIN);
    if (up_button_output == 1) {
        return UP;
    }
    int down_button_output = gpio_get_level(DOWN_BUTTON_PIN);
    if (down_button_output == 1) {
        return DOWN;
    }
    int toggle_drawing_button_output = gpio_get_level(TOGGLE_DRAWING_BUTTON_PIN);
    if (toggle_drawing_button_output == 1) {
        vTaskDelay(pdMS_TO_TICKS(250));
        return TOGGLE_DRAWING;
    }

    return NONE;
}
