#include "display.h"
#include "driver/gpio.h"
#include <string.h>

#define DISPLAY_OFFSET_Y 1
#define DISPLAY_OFFSET_X 2

#define LCD_HOST SPI3_HOST
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5
#define PIN_NUM_DC 2
#define PIN_NUM_RST 4

spi_device_handle_t display_spi_setup()
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
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2
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

void display_spi_write(spi_device_handle_t spi, const uint8_t* data, size_t data_length)
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

void display_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    uint8_t byte = 0;
    byte = cmd;
    gpio_set_level(PIN_NUM_DC, 0);
    display_spi_write(spi, &byte, 1);
}

void display_data(spi_device_handle_t spi, const uint8_t data)
{
    uint8_t byte = 0;
    byte = data;
    gpio_set_level(PIN_NUM_DC, 1);
    display_spi_write(spi, &byte, 1);
}

void display_pixel_write(spi_device_handle_t spi, uint8_t x, uint8_t y, uint16_t val)
{
    x = x + DISPLAY_OFFSET_X;
    y = y + DISPLAY_OFFSET_Y;
    uint8_t bytes[4];

    // X coordinate
    display_cmd(spi, 0x2a);
    bytes[0] = 0;
    bytes[1] = x;
    bytes[2] = 0;
    bytes[3] = x;
    gpio_set_level(PIN_NUM_DC, 1);
    display_spi_write(spi, bytes, 4);

    // Y coordinate
    display_cmd(spi, 0x2b);
    bytes[0] = 0;
    bytes[1] = y;
    bytes[2] = 0;
    bytes[3] = y;
    gpio_set_level(PIN_NUM_DC, 1);
    display_spi_write(spi, bytes, 4);

    // Val
    display_cmd(spi, 0x2c);
    bytes[0] = (val >> 8) & 0xff;
    bytes[1] = val & 0xff;
    gpio_set_level(PIN_NUM_DC, 1);
    display_spi_write(spi, bytes, 2);
}

void display_screen_fill(spi_device_handle_t spi, uint16_t val)
{
    uint8_t bytes[4];

    // X coordinate
    display_cmd(spi, 0x2a);
    bytes[0] = 0;
    bytes[1] = DISPLAY_OFFSET_X;
    bytes[2] = 0;
    bytes[3] = DISPLAY_OFFSET_X + DISPLAY_WIDTH - 1;
    gpio_set_level(PIN_NUM_DC, 1);
    display_spi_write(spi, bytes, 4);

    // Y coordinate
    display_cmd(spi, 0x2b);
    bytes[0] = 0;
    bytes[1] = DISPLAY_OFFSET_Y;
    bytes[2] = 0;
    bytes[3] = DISPLAY_OFFSET_Y + DISPLAY_HEIGHT - 1;
    gpio_set_level(PIN_NUM_DC, 1);
    display_spi_write(spi, bytes, 4);

    // Val
    display_cmd(spi, 0x2c);
    uint8_t* framebuffer = malloc(DISPLAY_HEIGHT * DISPLAY_WIDTH * 2);
    uint16_t little_endian_val = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
    for (int i = 0; i < DISPLAY_HEIGHT * DISPLAY_WIDTH; i++) {
        ((uint16_t*)framebuffer)[i] = little_endian_val;
    }
    gpio_set_level(PIN_NUM_DC, 1);
    display_spi_write(spi, framebuffer, DISPLAY_HEIGHT * DISPLAY_WIDTH * 2);
    free(framebuffer);
}

spi_device_handle_t display_init()
{
    spi_device_handle_t spi = display_spi_setup();
    display_cmd(spi, 0x01); // SW reset
    vTaskDelay(150 / portTICK_PERIOD_MS);

    display_cmd(spi, 0x11); // Sleep out
    vTaskDelay(255 / portTICK_PERIOD_MS);

    display_cmd(spi, 0x13); // Normal mode
    vTaskDelay(150 / portTICK_PERIOD_MS);

    display_cmd(spi, 0x29); // Display on
    vTaskDelay(255 / portTICK_PERIOD_MS);

    display_cmd(spi, 0xb1); // Framerate
    display_data(spi, 0x01);
    display_data(spi, 0x2c);
    display_data(spi, 0x2d);

    display_cmd(spi, 0x3a); // Pixel format
    display_data(spi, 0x05);

    display_cmd(spi, 0x36); // Memory read format
    display_data(spi, 0x8);

    return spi;
}
