#include <stdbool.h>
#include <stdint.h>

void lichess_api_init_client(void);
void lichess_api_login(const char* token, const uint16_t token_len);
void lichess_api_logout();
bool lichess_api_is_logged_in();
void lichess_api_get_email(void);
void lichess_api_make_move(char* move);
void lichess_api_stream_move_of_game();
void lichess_api_stream_event(void);
void lichess_api_create_game(bool rated, uint8_t minutes, uint8_t increment);
char* getColor();
char* lichess_api_get_username();