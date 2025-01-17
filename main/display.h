#include "driver/spi_master.h"
#include <stdbool.h>

#define DISPLAY_HEIGHT 128
#define DISPLAY_WIDTH 128

#define WHITE 0xffff
#define BLACK 0x0000
#define RED 0xf800
#define GREEN 0x7e0
#define BLUE 0x1f

spi_device_handle_t display_init();
void display_pixel_write(spi_device_handle_t spi, uint8_t x, uint8_t y, uint16_t val);
void display_screen_fill(spi_device_handle_t spi, uint16_t val);
