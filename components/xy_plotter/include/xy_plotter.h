#include <stdint.h>

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
