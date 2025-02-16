#include "display.h"
#include "esp_log.h"
#include "input.h"
#include "network.h"
#include <stdbool.h>

typedef struct drawcast_state_t {
    char draw_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

    int cursor_x;
    int cursor_y;
    bool drawing;

    uint16_t bg_color;
    uint16_t drawing_color;
    uint16_t cursor_color;
} drawcast_state;

typedef struct mqtt_message_receive_context_t {
    spi_device_handle_t display_spi;
    drawcast_state* state;
} mqtt_message_receive_context;

inline int min(int a, int b)
{
    return a > b ? b : a;
}

inline int max(int a, int b)
{
    return a < b ? b : a;
}

void publish_message_for_current_pixel(drawcast_state* state)
{
    char data[2];
    data[0] = state->cursor_x & 0x7f;
    data[1] = state->cursor_y & 0x7f;
    mqtt_send_message(data);
}

void receive_message(char* message_data, void* context)
{
    int cursor_x = message_data[0];
    int cursor_y = message_data[1];
    ESP_LOGI("MQTT_RCV", "x: %d, y: %d", cursor_x, cursor_y);
    mqtt_message_receive_context* ctx = (mqtt_message_receive_context*)context;
    ctx->state->draw_buffer[cursor_x][cursor_y] = 1;
    display_pixel_write(ctx->display_spi, cursor_x, cursor_y, ctx->state->drawing_color);
}

void handle_input(enum Input inp, spi_device_handle_t display_spi, drawcast_state* state)
{
    if (inp == TOGGLE_DRAWING) {
        state->drawing = !state->drawing;
        if (state->drawing) {
            state->draw_buffer[state->cursor_y][state->cursor_x] = 1;
            publish_message_for_current_pixel(state);
        }
        ESP_LOGI("DIRECTION", "Drawing flag set to: %d", state->drawing);
        return;
    }

    // Before moving cursor, reset pixel value to existing state
    if (state->draw_buffer[state->cursor_y][state->cursor_x]) {
        display_pixel_write(display_spi, state->cursor_x, state->cursor_y, state->drawing_color);
    } else {
        display_pixel_write(display_spi, state->cursor_x, state->cursor_y, state->bg_color);
    }

    if (inp == UP) {
        ESP_LOGI("DIRECTION", "Up");
        state->cursor_y = max(0, state->cursor_y - 1);
    } else if (inp == LEFT) {
        ESP_LOGI("DIRECTION", "Left");
        state->cursor_x = max(0, state->cursor_x - 1);
    } else if (inp == DOWN) {
        ESP_LOGI("DIRECTION", "Down");
        state->cursor_y = min(DISPLAY_HEIGHT - 1, state->cursor_y + 1);
    } else if (inp == RIGHT) {
        ESP_LOGI("DIRECTION", "Right");
        state->cursor_x = min(DISPLAY_WIDTH - 1, state->cursor_x + 1);
    }

    ESP_LOGI("DIRECTION", "New coords: %d, %d", state->cursor_y, state->cursor_x);
    display_pixel_write(display_spi, state->cursor_x, state->cursor_y, state->cursor_color);

    if (!state->drawing) {
        return;
    }
    state->draw_buffer[state->cursor_y][state->cursor_x] = 1;
    publish_message_for_current_pixel(state);
}

void handle_input_bitset(input_bitset inp_bits, spi_device_handle_t display_spi, drawcast_state* state)
{
    for (int i = 0; i < NUM_INPUT_TYPES; i++) {
        Input input = input_elems[i];
        if (inp_bits & (1 << input)) {
            handle_input(input, display_spi, state);
        }
    }
}

void init_state(drawcast_state* state)
{
    state->cursor_x = DISPLAY_WIDTH / 2;
    state->cursor_y = DISPLAY_HEIGHT / 2;
    state->drawing = false;

    state->bg_color = BLACK;
    state->drawing_color = WHITE;
    state->cursor_color = RED;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_HEIGHT; x++) {
            state->draw_buffer[y][x] = 0;
        }
    }
}

void app_main(void)
{
    drawcast_state* drawcast_state = malloc(sizeof(struct drawcast_state_t));
    init_state(drawcast_state);

    setup_uart_input(); // Keyboard input from computer;
    setup_push_button_input();

    // Setup ST7735S LCD
    spi_device_handle_t display_spi = display_init();
    display_screen_fill(display_spi, drawcast_state->bg_color);
    display_pixel_write(display_spi, drawcast_state->cursor_x, drawcast_state->cursor_y, drawcast_state->cursor_color);

    // Networking
    wifi_connect();
    mqtt_message_receive_context msg_rcv_ctx;
    msg_rcv_ctx.display_spi = display_spi;
    msg_rcv_ctx.state = drawcast_state;
    mqtt_connect(receive_message, &msg_rcv_ctx);

    ESP_LOGI("MAIN", "Ready!");
    while (1) {
        input_bitset inp_bits = handle_uart_input();
        handle_input_bitset(inp_bits, display_spi, drawcast_state);
        inp_bits = handle_push_button_input();
        handle_input_bitset(inp_bits, display_spi, drawcast_state);
        vTaskDelay(pdMS_TO_TICKS(16));
    }

    free(drawcast_state);
}
