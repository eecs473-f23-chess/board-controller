#ifndef TYPES_H
#define TYPES_H

typedef enum PIECE_COLOR {
    WHITE,
    BLACK,
    NONE,
} piece_color_t;

typedef enum OPPONENT_TYPE {
    RANDOM_PLAYER,
    SPECIFIC_PLAYER
} opponent_type_t;

typedef enum MOVE_TYPE {
    CASTLE,
    EN_PASSANT,
    NORMAL
} move_type_t;

typedef enum {
    NP=0, 
    WR, 
    WN, 
    WB, 
    WK, 
    WQ, 
    WP, 
    BRK, 
    BN, 
    BB, 
    BK, 
    BQ, 
    BP
} Board;

#endif
