#include <stdint.h>
#include <types.h>
#include "board_state.h"
#include "lichess_api.h"

struct target_square {
    uint8_t target_x_cord;
    uint8_t target_y_cord;
    piece_color_t target_emag_status;
};

struct move_sequence {
    uint8_t num_moves;
    struct target_square squares_to_move[10];
};

// Test XY plotter with joystick connected to GPIO19 and GPIO20
// #define XYP_JOYSTICK_TEST

void xyp_init();
void xyp_set_board_pos(const float x_pos, const float y_pos);

void xyp_calibrate();

// setting my guy in the middle of the board
void xyp_return_home();

#ifdef XYP_JOYSTICK_TEST
void xyp_joystick_control();
#endif
struct move_sequence generate_moves(Board current_state[8][8]);
