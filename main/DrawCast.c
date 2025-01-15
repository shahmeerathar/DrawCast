#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

#include "esp_log.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

#define DISPLAY_HEIGHT 128
#define DISPLAY_WIDTH 128
#define DISPLAY_OFFSET_Y 2
#define DISPLAY_OFFSET_X 1

#define LCD_HOST SPI3_HOST
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5
#define PIN_NUM_DC 2
#define PIN_NUM_RST 4

#define WHITE 0xffff
#define BLACK 0x0000

char draw_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
int cursor_x;
int cursor_y;
bool drawing;

inline int min(int a, int b)
{
    return a > b ? b : a;
}

inline int max(int a, int b)
{
    return a < b ? b : a;
}

spi_device_handle_t lcd_spi_setup()
{
    gpio_reset_pin(PIN_NUM_CS);
    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_CS, 0);

    gpio_reset_pin(PIN_NUM_DC);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_DC, 0);

    gpio_reset_pin(PIN_NUM_RST);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1);

    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
    };
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    return spi;
}

void lcd_spi_write(spi_device_handle_t spi, const uint8_t* data, size_t data_length)
{
    spi_transaction_t spi_transaction;
    esp_err_t ret;

    if (data_length > 0) {
        memset(&spi_transaction, 0, sizeof(spi_transaction_t));
        spi_transaction.length = data_length * 8;
        spi_transaction.tx_buffer = data;
        ret = spi_device_transmit(spi, &spi_transaction);
        ESP_ERROR_CHECK(ret);
    }
}

void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    uint8_t byte = 0;
    byte = cmd;
    gpio_set_level(PIN_NUM_DC, 0);
    lcd_spi_write(spi, &byte, 1);
}

void lcd_data(spi_device_handle_t spi, const uint8_t data)
{
    uint8_t byte = 0;
    byte = data;
    gpio_set_level(PIN_NUM_DC, 1);
    lcd_spi_write(spi, &byte, 1);
}

void lcd_pixel_write(spi_device_handle_t spi, uint8_t x, uint8_t y, uint16_t val)
{
    x = x + DISPLAY_OFFSET_X;
    y = y + DISPLAY_OFFSET_Y;

    // Y coordinate
    gpio_set_level(PIN_NUM_DC, 0);
    lcd_cmd(spi, 0x2a);
    gpio_set_level(PIN_NUM_DC, 1);
    uint8_t bytes[4];
    bytes[0] = 0;
    bytes[1] = y;
    bytes[2] = 0;
    bytes[3] = y;
    lcd_spi_write(spi, bytes, 4);

    // X coordinate
    gpio_set_level(PIN_NUM_DC, 0);
    lcd_cmd(spi, 0x2b);
    gpio_set_level(PIN_NUM_DC, 1);
    bytes[0] = 0;
    bytes[1] = x;
    bytes[2] = 0;
    bytes[3] = x;
    lcd_spi_write(spi, bytes, 4);

    // Val
    gpio_set_level(PIN_NUM_DC, 0);
    lcd_cmd(spi, 0x2c);
    gpio_set_level(PIN_NUM_DC, 1);
    bytes[0] = (val >> 8) & 0xff;
    bytes[1] = val & 0xff;
    lcd_spi_write(spi, bytes, 2);
}

void lcd_screen_fill(spi_device_handle_t spi, uint16_t val)
{
    for (uint8_t y = 0; y < DISPLAY_HEIGHT; y++) {
        for (uint8_t x = 0; x < DISPLAY_WIDTH; x++) {
            lcd_pixel_write(spi, x, y, val);
        }
    }
}

void lcd_init(spi_device_handle_t spi)
{
    lcd_cmd(spi, 0x01); // SW reset
    vTaskDelay(150 / portTICK_PERIOD_MS);

    lcd_cmd(spi, 0x11); // Sleep out
    vTaskDelay(255 / portTICK_PERIOD_MS);

    lcd_cmd(spi, 0x13); // Normal mode
    vTaskDelay(150 / portTICK_PERIOD_MS);

    lcd_cmd(spi, 0x29); // Display on
    vTaskDelay(255 / portTICK_PERIOD_MS);

    lcd_cmd(spi, 0xb1); // Framerate
    lcd_data(spi, 0x01);
    lcd_data(spi, 0x2c);
    lcd_data(spi, 0x2d);

    lcd_cmd(spi, 0x3a); // Pixel format
    lcd_data(spi, 0x05);

    lcd_cmd(spi, 0x36); // Memory read format
    lcd_data(spi, 0x40);

    lcd_screen_fill(spi, BLACK);
}

void handle_input(char input, spi_device_handle_t spi)
{
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

    if (!drawing) {
        return;
    }
    draw_buffer[cursor_y][cursor_x] = 1;
    lcd_pixel_write(spi, cursor_x, cursor_y, WHITE);
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

    // Keyboard input from computer
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);

    // Setup ST7735S LCD
    spi_device_handle_t spi = lcd_spi_setup();
    lcd_init(spi);
    ESP_LOGI("MAIN", "Ready!");

    uint8_t data[BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len == 1) {
            handle_input(data[0], spi);
        }
    }
}
