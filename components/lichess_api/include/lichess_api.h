#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/*************************************************************/
// These functions deal directly with the lichess api
void lichess_api_init_client(void);
void lichess_api_login(const char* token, const uint16_t token_len);
void lichess_api_logout();
bool lichess_api_is_logged_in();
void lichess_api_get_email(void);
void lichess_api_make_move(char* move);
void lichess_api_stream_move_of_game();
void lichess_api_stream_event(void);
void lichess_api_create_game(bool rated, uint8_t minutes, uint8_t increment, bool white);
void lichess_api_get_account_info(void);
void lichess_api_handle_draw(void);
void lichess_api_resign_game(void);
char* lichess_api_get_username(void);
/*************************************************************/


/*************************************************************/

// These are helper functions that deal with semaphores (tasks)
void lichess_api_create_game_helper(void);
void lichess_api_resign_game_helper(void);
void lichess_api_handle_draw_helper(void);
void lichess_api_make_move_helper(void);
/*************************************************************/


/*************************************************************/

// These functions are helpers that deal with instantiated variables in the program
char* getColor();
bool get_opponent_move_update();
void reset_opponent_move_update();
char* get_last_move_played_by_opponent();
char* get_user_country(char* username);
<<<<<<< HEAD
void lichess_api_resign_game();
void lichess_api_handle_draw();
=======
/*************************************************************/
>>>>>>> 826f7e6 (Clock button for making a move works)
