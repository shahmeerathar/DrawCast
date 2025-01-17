#define NUM_INPUT_TYPES 5

typedef enum Input {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    TOGGLE_DRAWING,
} Input;

typedef char input_bitset;
extern const Input input_elems[NUM_INPUT_TYPES];

void setup_uart_input();
void setup_push_button_input();
input_bitset handle_uart_input();
input_bitset handle_push_button_input();
