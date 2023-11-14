#include <stdint.h>

void xyp_init();
void xyp_set_board_pos(const float x_pos, const float y_pos);

void xyp_calibrate();

// setting my guy in the middle of the board
void xyp_return_home();