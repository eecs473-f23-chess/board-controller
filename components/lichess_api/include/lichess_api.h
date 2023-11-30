#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "types.h"
#include "board_state.h"
#include "Buttons.h"

#ifndef LICHESS_API_H
#define LICHESS_API_H

typedef enum TIME_CONTROL {
        TC_10_0 = 0,
        TC_10_5 = 1,
        TC_15_10 = 2,
        TC_30_0 = 3,
        TC_30_20 = 4,
} time_control_t;

/*************************************************************/
// These functions deal directly with the lichess api
void lichess_api_init_client(void);
void lichess_api_login(const char* token, const uint16_t token_len);
void lichess_api_logout();
bool lichess_api_is_logged_in();
void lichess_api_get_email(void);
void lichess_api_set_time_control(time_control_t tc);
void lichess_api_get_time_control(time_control_t* tc);
void lichess_api_set_opponent_type(opponent_type_t opponent_type);
void lichess_api_get_opponent_type(opponent_type_t* opponent_type);
void lichess_api_set_specific_username(char* specific_username, uint16_t len);
void lichess_api_get_specific_username(char** username);
void lichess_api_make_move(char* move);
void lichess_api_stream_move_of_game(void *pvParameters);
void lichess_api_stream_event(void);
void lichess_api_create_game(bool rated);
void lichess_api_get_account_info(void);
void lichess_api_handle_draw(void);
void lichess_api_resign_game(void);
char* lichess_api_get_username(void);
void lichess_api_set_user_country(char* other_user);
void lichess_api_set_user_board_state(board_state_t* board_state);
/*************************************************************/


/*************************************************************/

// These are helper functions that deal with semaphores (tasks)
void lichess_api_create_game_helper(void *pvParameters);
void lichess_api_resign_game_helper(void*pvParameters);
void lichess_api_handle_draw_helper(void*pvParameters);
void lichess_api_make_move_helper(void *pvParameters);
/*************************************************************/


/*************************************************************/

// These functions are helpers that deal with instantiated variables in the program
char* getColor();
bool get_opponent_move_update();
void reset_opponent_move_update();
char* get_last_move_played_by_opponent();
char* get_user_country(char* username);
/*************************************************************/


/*************************************************************/
// These variables are used amongst other files
extern bool game_created;
/*************************************************************/

#endif
