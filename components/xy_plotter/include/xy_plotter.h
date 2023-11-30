#include <stdint.h>
#include "board_state.h"

struct target_square {
    float target_x_cord;
    float target_y_cord;
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

// check electrmagnet status before every move when using generate_moves()
void xyp_generate_moves(struct move_sequence * sequence, board_state_t* board_state, const move_type_t move_type, char * move_to_make);
void xyp_play_move(struct move_sequence* sequence);
