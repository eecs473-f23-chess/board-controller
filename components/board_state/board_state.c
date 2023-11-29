#include "board_state.h"

/*
    ENUMs for chess board:
        NP -> No piece
    
        Starts with W -> White piece
        Starts witH B -> Black piece

        K -> King
        Q -> Queen
        B -> Bishop
        N -> Knight (Had to use N cause K is taken)
        R -> Rook
        P -> Pawn
*/

// TODO, For some reason ESP didn't like BR as an ENUM so I name it BRK (black rook)
Board chess_board [8][8];
#define CHESS_ROWS 8
#define CHESS_COLS 8


// Helper function to print current state of board
void board_state_print(){
    for(int i = 0; i < CHESS_ROWS; i++){
        for(int j = 0; j < CHESS_COLS; j++){
            printf("%2d ", chess_board[i][j]);
        }
        printf("\n");
    }
}

// Sets up the chess board with initial state (White pieces on bottom)
void board_state_init(){
    // Setting up the black pieces

    for(int i = 0; i < 2; i++){
        for(int j = 0; j < CHESS_COLS; j++){
            if(i == 1){
                chess_board[i][j] = BP;
            }
            else{
                if(j == 0 || j == 7){
                    chess_board[i][j] = BRK;
                }
                else if(j == 1 || j == 6){
                    chess_board[i][j] = BN;
                }
                else if(j == 2 || j == 5){
                    chess_board[i][j] = BB;
                }
                else if(j == 3){
                    chess_board[i][j] = BQ;
                }
                else{
                    // j has to be 4
                    chess_board[i][j] = BK;
                }
            }
        }
    }
    
    for(int i = 2; i <= 5; i++){
        for(int j = 0; j < CHESS_COLS; j++){
            chess_board[i][j] = NP;
        }
    }

    // Setting up white pieces
    for(int i = 6; i < CHESS_ROWS; i++){
        for(int j = 0; j < CHESS_COLS; j++){
            if(i == 6){
                chess_board[i][j] = WP;
            }
            else{
                if(j == 0 || j == 7){
                    chess_board[i][j] = WR;
                }
                else if(j == 1 || j == 6){
                    chess_board[i][j] = WN;
                }
                else if(j == 2 || j == 5){
                    chess_board[i][j] = WB;
                }
                else if(j == 3){
                    chess_board[i][j] = WQ;
                }
                else{
                    // j has to be 4
                    chess_board[i][j] = WK;
                }
            }
        }
    }
}

// Maps array coordinate (0,0) to chess notation (a1)
void board_state_array_coord_to_chess_not(int x, int y, char* move){
    int rank = 8 - x;
    char rank_as_char = (char)(rank + '0');
    char file = (char)(y + 'a');
    char coordinate[3] = {};
    coordinate[0] = file;
    coordinate[1] = rank_as_char;
    coordinate[2] = 0;
    strcpy(move, coordinate);
}

/*
    Returns chess piece on a specific row and column
*/
int board_state_get_piece_on_square(int row, int col){
    return chess_board[row][col];
}

/* Returns if two square are diagonally 1 apart */
bool board_state_diag_dist_one(int x, int y, int x1, int y1){
    return ((abs(x - x1) == 1) && (abs(y - y1) == 1));
}
/*
    Sets a chess piece on a square
*/
void board_state_set_chess_piece_on_square(int row, int col, Board piece){
    chess_board[row][col] = piece;
}
/*
     Update chess board based on opponent move.
     Don't need to worry about illegal moves, since lichess takes care of that for us
*/
void board_state_update_board_based_on_opponent_move(char* move){
    printf("Move is %s\n", move);
    int n = strlen(move);

    char src_file = move[0]; char src_rank = move[1];
    char dest_file = move[2]; char dest_rank = move[3];

    int src_x = CHESS_COLS - (src_rank - '0'); int src_y = (src_file - 'a');
    int dest_x = CHESS_COLS - (dest_rank - '0');int dest_y = (dest_file - 'a');

    if (n == 5){
        char piece_to_promote = move[4];
        
        Board source_piece = board_state_get_piece_on_square(src_x, src_y);

        if (source_piece != WP && source_piece != BP){
            printf("ERROR: Piece on source square isn't a white or black pawn. Cannot promote!\n");
            return;
        }

        if (!(piece_to_promote == 'q' || piece_to_promote == 'n' || piece_to_promote == 'r' || piece_to_promote == 'b')){
            printf("ERROR: Pawn can only promote to queen (q), rook (r), knight (n), or bishop (b)\n");
            return;
        }

        if (source_piece == WP){
            board_state_set_chess_piece_on_square(src_x, src_y, NP);
            if(piece_to_promote == 'q'){
                board_state_set_chess_piece_on_square(dest_x, dest_y, WQ);
            }
            else if (piece_to_promote == 'r'){
                board_state_set_chess_piece_on_square(dest_x, dest_y, WR);
            }
            else if (piece_to_promote == 'n'){                
                board_state_set_chess_piece_on_square(dest_x, dest_y, WN);
            }
            else{
                // Based off error checking above, has to be bishop
                board_state_set_chess_piece_on_square(dest_x, dest_y, WB);
            }
        }

        // Based off error checking above, has to be black pawn
        else {
            board_state_set_chess_piece_on_square(src_x, src_y, NP);
            if(piece_to_promote == 'q'){
                board_state_set_chess_piece_on_square(dest_x, dest_y, BQ);
            }
            else if (piece_to_promote == 'r'){
                board_state_set_chess_piece_on_square(dest_x, dest_y, BRK);
            }
            else if (piece_to_promote == 'n'){                
                board_state_set_chess_piece_on_square(dest_x, dest_y, BN);
            }
            else{
                // Based off error checking above, has to be bishop
                board_state_set_chess_piece_on_square(dest_x, dest_y, BB);
            }
        }
        // TODO: ADD PROMOTION ENUM HERE. 
        return;
    }
    if(n != 4){
        printf("Error in receiving move from lichess. Expected 4 character move, and got %d char move. GOT %s\n", n, move);
        return;
    }

   

    /*
        Castling: There are only 4 castling options.
        White
            Kingside: e1g1
            Queenside: e1c1
        
        Black
            Kingside: e8g8
            Queenside: e8c8
        
        If we get any of these, we first check if there is a king on e1 or e8,
        and if there is, we KNOW its castling (lichess wouldn't have given us the move). 

        King cannot legally move e file to g file in one move, so if there is a king
        it has to be castled. 

        If its not a king, then we aren't castling and move down
    */
    char* white_king = "e1g1";
    char* white_queen = "e1c1";

    char* black_king = "e8g8";
    char* black_queen = "e8c8";

    if(strcmp(move, white_king) == 0){
        // If the source square contains a white king
        if(board_state_get_piece_on_square(src_x, src_y) == WK){
            // There has to be a white rook on h1 (7,7)
            if(board_state_get_piece_on_square(dest_x, dest_y + 1) != WR){
                printf("ERROR. White kingside castling, not rook on h1!\n");
                return;
            }
            else{
                board_state_set_chess_piece_on_square(src_x, src_y, NP);
                board_state_set_chess_piece_on_square(dest_x, dest_y + 1, NP);
                board_state_set_chess_piece_on_square(dest_x, dest_y, WK);
                board_state_set_chess_piece_on_square(dest_x, dest_y - 1, WR);
                return;
            }
        }
    }
    
    else if(strcmp(move, white_queen) == 0){
        if(board_state_get_piece_on_square(src_x, src_y) == WK){
            // There has to be a white rook on a1 (7,0)
            if(board_state_get_piece_on_square(7, 0) != WR){
                printf("ERROR. White queenside castling, not rook on a1!\n");
                return;
            }
            else{
                board_state_set_chess_piece_on_square(src_x, src_y, NP);
                board_state_set_chess_piece_on_square(7,0,NP);
                board_state_set_chess_piece_on_square(dest_x, dest_y, WK);
                board_state_set_chess_piece_on_square(dest_x, dest_y + 1, WR);
                return;
            }
        }
    }
    
    else if(strcmp(move, black_king) == 0){
        if(board_state_get_piece_on_square(src_x, src_y) == BK){
            // There has to be a black rook on h8 (0,7)
            if(board_state_get_piece_on_square(0, 7) != BRK){
                printf("ERROR. Black kingside castling, not rook on h8!\n");
                return;
            }
            else{
                board_state_set_chess_piece_on_square(src_x, src_y, NP);
                board_state_set_chess_piece_on_square(0,7,NP);
                board_state_set_chess_piece_on_square(dest_x, dest_y, BK);
                board_state_set_chess_piece_on_square(dest_x, dest_y - 1, BRK);
                return;
            }
        }
    }
    
    else if(strcmp(move, black_queen) == 0){
        if(board_state_get_piece_on_square(src_x, src_y) == BK){
            // There has to be a black rook on a8 (0,0)
            if(board_state_get_piece_on_square(0, 0) != BRK){
                printf("ERROR. Black queenside castling, not rook on a8!\n");
                return;
            }
            else{
                board_state_set_chess_piece_on_square(src_x, src_y, NP);
                board_state_set_chess_piece_on_square(0,0 ,NP);
                board_state_set_chess_piece_on_square(dest_x, dest_y, BK);
                board_state_set_chess_piece_on_square(dest_x, dest_y + 1, BRK);
                return;
            }
        }
    }


    /*
        EN PASSANT

        1) The src square has to have a pawn (either white or black)
        2) The dest square has to be empty
        3) The src and dest has to be diagonal with distance 1
        4) Check the y coordinate
            a) For (sx, sy) (dx, dy) if dy = sy + 1 then sx, dy+1 has to have the opposite color pawn
                                     if dy = sx - 1, then sx, dy-1 has to have the opposite color pawn
    */

   // If there is a white pawn
   if(board_state_get_piece_on_square(src_x, src_y) == WP){

        // If there is no piece on the destination square
        if(board_state_get_piece_on_square(dest_x, dest_y) == NP){

            // The destination has to be 1 diagonal square away
            if(board_state_diag_dist_one(src_x, src_y, dest_x, dest_y)){
                if(board_state_get_piece_on_square(src_x, dest_y) != BP){
                    printf("Expected black pawn next to white pawn\n");
                    return;
                }
                else{
                    board_state_set_chess_piece_on_square(src_x, src_y, NP);
                    board_state_set_chess_piece_on_square(dest_x, dest_y, WP);
                    board_state_set_chess_piece_on_square(src_x, dest_y, NP);
                    return;
                }
            }
        }
   }
   
   else if(board_state_get_piece_on_square(src_x, src_y) == BP){
     // If there is no piece on the destination square
        if(board_state_get_piece_on_square(dest_x, dest_y) == NP){

            // The destination has to be 1 diagonal square away
            if(board_state_diag_dist_one(src_x, src_y, dest_x, dest_y)){
                if(board_state_get_piece_on_square(src_x, dest_y) != WP){
                    printf("Expected white pawn next to black pawn\n");
                    return;
                }
                else{
                    board_state_set_chess_piece_on_square(src_x, src_y, NP);
                    board_state_set_chess_piece_on_square(dest_x, dest_y, BP);
                    board_state_set_chess_piece_on_square(src_x, dest_y, NP);
                    return;
                }
            }
        }
   }

    // If we reached here, should be a normal move (capture, or moving a piece)
    Board piece_to_move = board_state_get_piece_on_square(src_x, src_y);
    board_state_set_chess_piece_on_square(src_x, src_y, NP);
    board_state_set_chess_piece_on_square(dest_x, dest_y, piece_to_move);
    return;
}


