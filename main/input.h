typedef enum Input {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    TOGGLE_DRAWING,
    NONE
} Input;

void setup_uart_input();
void setup_push_button_input();
Input handle_uart_input();
Input handle_push_button_input();
