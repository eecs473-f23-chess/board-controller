#ifndef BOARD_STATE_H
#define BOARD_STATE_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"

typedef struct BOARD_STATE {
    Board board[8][8];
} board_state_t;

void board_state_print();
void board_state_init();
void board_state_array_coord_to_chess_not(int x, int y, char* move);
int board_state_get_piece_on_square(board_state_t* board_state, int row, int col);
bool board_state_diag_dist_one(int x, int y, int x1, int y1);
void board_state_set_chess_piece_on_square(board_state_t* board_state, int row, int col, Board piece);
bool board_state_update_board_based_on_opponent_move(char* move, move_type_t * move_type);
board_state_t* board_state_get_current_board_state();
#endif