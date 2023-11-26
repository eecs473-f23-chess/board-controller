#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"

void board_state_print();
void board_state_init();
void board_state_array_coord_to_chess_not(int x, int y, char* move);
int board_state_get_piece_on_square(int row, int col);
bool board_state_diag_dist_one(int x, int y, int x1, int y1);
void board_state_set_chess_piece_on_square(int row, int col, Board piece);
void board_state_update_board_based_on_opponent_move(char* move, move_type_t * move_type);