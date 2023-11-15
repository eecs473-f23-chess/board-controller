#include "lichess_api.h"

#include <esp_http_client.h>
#include <esp_log.h>
#include <math.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <string.h> 
#include <cJSON.h>

#include "../clock_display/include/clock_display.h"     
#include "../score_display/include/score_display.h"
#include "wifi.h"
#include "Buttons.h"

#define MAX_HTTP_OUTPUT_BUFFER  4096
#define AUTHORIZATION_HEADER    "Authorization"
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

// Used to wait indefinetly until lichess game is created
#define INCLUDE_vTaskSuspend 1

static char GAME_ID[100] = {};
static char FULL_ID[100] = {};
static char last_move_played_by_opponent[5] = {};
static char response_buf[2000] = {};
static char color[10] = {};
static char user_name[100] = {};
static char rating[5] = {};
static char country[5] = {};
static char bearer_token[64] = {};
static bool logged_in;
static bool move_update = false;
static bool want_moves = false;
static bool draw_has_been_offered = false;
uint32_t white_time = -1;
uint32_t black_time = -1;
static char opponent_username[100] = {};
static char opponent_rating[5] = {};
static char opponent_country[5] = {};
static esp_http_client_handle_t client;
static esp_http_client_handle_t client_stream;


static SemaphoreHandle_t xSemaphore_API;
SemaphoreHandle_t xSemaphore_DataTransfer;
extern bool game_created;


esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static const char* TAG = "HTTP_EVENT_HANDLER";
    static char *output_buffer;  
    static int output_len;          
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
                        break;
        case HTTP_EVENT_ON_DATA:
            sleep(0.3);
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d\n", evt->data_len);
            printf("Event data addy: %p\n", evt->data);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                    printf("Output buffer %s\n", output_buffer);
                }
                output_len += copy_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

static esp_http_client_config_t config = {
        .url = "https://lichess.org/api/",
        .path = "/get",
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
        .event_handler = _http_event_handler,
        .user_data = response_buf,
};

char* getColor(){
    return color;
}

void set_username(const char *json_str){
    cJSON *root = cJSON_Parse(json_str);    
    if (root == NULL) {
        printf("{set_username} Error parsing JSON.\n");
        return;
    }
    cJSON *user = cJSON_GetObjectItem(root, "username");
    if(user == NULL){
        printf("{set_username} username isn't defined in root\n");
        cJSON_Delete(root);
        return;
    }
    for(int i = 0; i < strlen(user->valuestring); i++){
        user_name[i] = (user->valuestring)[i];
    }
    cJSON_Delete(root);
}

void set_rating(char *json){
    cJSON *root = cJSON_Parse(json);    
    if (root == NULL) {
        printf("{set_rating} Error parsing JSON.\n");
        return;
    }
    cJSON *per = cJSON_GetObjectItem(root, "perfs");
    if(per == NULL){
        printf("{set_rating} per isn't defined\n");
        cJSON_Delete(root);
        return;
    }
    cJSON *rap = cJSON_GetObjectItem(per, "rapid");
    if(rap == NULL){
        printf("{set_rating} rapid isn't defined in root\n");
        cJSON_Delete(root);
        return;
    }
    cJSON *rat = cJSON_GetObjectItem(rap, "rating");
     if(rat == NULL){
        printf("{set_rating} rating isn't defined in root\n");
        cJSON_Delete(root);
        return;
    }
    int actual_rapid_rating = rat->valueint;
    sprintf(rating, "%d", actual_rapid_rating);
    cJSON_Delete(root);
}

// TODO, Make it one function so we can parse it once. 
void set_game_id(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);    
    if (root == NULL) {
        printf("{set game id} ROOT JSON IS NULL.\n");
        return;
    }
    cJSON *game = cJSON_GetObjectItem(root, "game");
    cJSON *gameId = cJSON_GetObjectItem(game, "gameId");
    cJSON *fullId = cJSON_GetObjectItem(game, "fullId");
    for(int i = 0; i < strlen(gameId->valuestring); i++){
        GAME_ID[i] = (gameId->valuestring)[i];
    }
    for(int i = 0; i < strlen(fullId->valuestring); i++){
        FULL_ID[i] = (fullId->valuestring)[i];
    }
    cJSON_Delete(root);
}

void set_color(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);    
    if (root == NULL) {
        printf("Error parsing JSON.\n");
        return;
    }
    cJSON *game = cJSON_GetObjectItem(root, "game");
    cJSON *c = cJSON_GetObjectItem(game, "color");
    for(int i = 0; i < strlen(c->valuestring); i++){
        color[i] = (c->valuestring)[i];
    }
    cJSON_Delete(root);
}

void set_last_move_played_by_opponent(char* json){
    cJSON *root = cJSON_Parse(json);
    if(root == NULL){
        printf("{set_last_move_played_by_opponent} ERROR PARSING JSON\n");
        return;
    }
    cJSON *type = cJSON_GetObjectItem(root, "type");
    cJSON *moves = NULL;
    if(strcmp(type->valuestring,"gameFull") == 0){
        cJSON *state = cJSON_GetObjectItem(root, "state");
        moves = cJSON_GetObjectItem(state, "moves");        
    }
    else if(strcmp(type->valuestring,"gameState") == 0){
        moves = cJSON_GetObjectItem(root, "moves");
    }
    else{
        printf("{set_last_move_played_by_opponent} Type isn't gameState of gameFull\n");
        cJSON_Delete(root);    
        return;
    }
    char* full_moves = moves->valuestring;
    int n = strlen(full_moves);
    if(n == 0){
        last_move_played_by_opponent[0] = 'N';
        last_move_played_by_opponent[1] = '/';
        last_move_played_by_opponent[2] = 'A';
    }
    else{
        for(int i = n-4; i < n; i++){
            last_move_played_by_opponent[i-n+4] = full_moves[i];
        }
    }
    cJSON_Delete(root);
}

char* check_result_of_game(char *json){
    char* game_in_progress = "GAME IN PROGRESS";
    char* white_resigned = "0-1 (Black wins)";
    char* black_resigned = "1-0 (White wins)";
    cJSON *root = cJSON_Parse(json);
    if(root == NULL){
        printf("{check_result_of_game} ERROR PARSING JSON\n");
        return NULL;
    }
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if(type == NULL){
        printf("{check_result_of_game} Type error in check result\n");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON *status = NULL;
    char* game_status = NULL;
    if(strcmp(type->valuestring, "gameFull") == 0){
        status = cJSON_GetObjectItem(root, "state");
        if(status == NULL){
            printf("{check_result_of_game} Status is null\n");
            cJSON_Delete(root);
            return NULL;
        }
    }
    else if(strcmp(type->valuestring, "gameState") == 0) {
        status = cJSON_GetObjectItem(root, "status");
        cJSON* draw_potentialb = cJSON_GetObjectItem(root, "bdraw");
        if(draw_potentialb != NULL){
            printf("Black has offered a draw!\n");
            scoreboard_OfferDraw(true);
        }
        cJSON* draw_potentialw = cJSON_GetObjectItem(root, "wdraw");
        if(draw_potentialw != NULL){
            printf("White has offered a draw!\n");
            scoreboard_OfferDraw(false);
        }
        if(status == NULL){
            printf("{check_result_of_game} Status is null in get result\n");
        }
        game_status = status->valuestring;
    }
    else if(strcmp(type->valuestring, "chatLine") == 0){
        cJSON* text = cJSON_GetObjectItem(root, "text");
        char* firstOption = "Draw offer accepted";
        if(strcmp(firstOption, text->valuestring) == 0){
            cJSON_Delete(root);
            return "1/2 - 1/2. Game drawn";
        }
        else{
            char* txtVal = text->valuestring;
            cJSON_Delete(root);
            return txtVal;
        }
    }
    if(strcmp(type->valuestring, "gameFull") == 0){
        cJSON* real_status = cJSON_GetObjectItem(status, "status");
        game_status = real_status->valuestring;
    }
    if(game_status == NULL){
        printf("{check_result_of_game} Game status is null\n");
        cJSON_Delete(root);
        return NULL;
    }
    if(strcmp(game_status, "started") == 0){
        cJSON_Delete(root);
        return game_in_progress;
    }
    else if(strcmp(game_status, "resign") == 0 || strcmp(game_status, "mate") == 0){
        cJSON *winner = cJSON_GetObjectItem(root, "winner");
        char* winner_str = winner->valuestring;
        if(strcmp(winner_str, "white") == 0){
            cJSON_Delete(root);
            return black_resigned;
        }
        if(strcmp(winner_str, "black") == 0){
            cJSON_Delete(root);
            return white_resigned;
        }
    }
    cJSON_Delete(root);
    return "{check_result_of_game} DIDNT RETURN ANYTHING";    
}

char* get_last_move_played_by_opponent(){
    return last_move_played_by_opponent;
}

void set_clock_time(char *json){
    cJSON *root = cJSON_Parse(json);
    if(root == NULL){
        printf("ERROR in set_clock_time\n");
        return;
    }
    cJSON *type = cJSON_GetObjectItem(root, "type");

    if(strcmp(type->valuestring, "gameFull") == 0){
        cJSON *state = cJSON_GetObjectItem(root, "state");
        cJSON *wtime = cJSON_GetObjectItem(state, "wtime");
        cJSON *btime = cJSON_GetObjectItem(state, "btime");
        white_time = wtime->valueint;
        black_time = btime->valueint;
    }
    else if(strcmp(type->valuestring, "gameState") == 0){
        cJSON *wtime = cJSON_GetObjectItem(root, "wtime");
        cJSON *btime = cJSON_GetObjectItem(root, "btime");
        white_time = wtime->valueint;
        black_time = btime->valueint;
    }
    else{
        printf("{set_clock_time} Type isn't gameState of gameFull\n");
    }
    cJSON_Delete(root);
}

char* lichess_api_get_username() {
    return user_name;
}

bool get_opponent_move_update(){
    return move_update;
}

void reset_opponent_move_update(){
    move_update = false;
}

void set_opponent_username_and_rating(char* json){
    cJSON *root = cJSON_Parse(json);
    cJSON *game_all = cJSON_GetObjectItem(root, "game");
    cJSON *opponent_all = cJSON_GetObjectItem(game_all, "opponent");
    if(opponent_all == NULL){
        printf("{set_opponent_username_and_rating} json doesn't have opponent information\n");
        cJSON_Delete(root);
        return;
    }
    cJSON *opponent_user = cJSON_GetObjectItem(opponent_all, "username");
    cJSON *opponent_rater = cJSON_GetObjectItem(opponent_all, "rating");
    if(opponent_user == NULL){
        printf("{set_opponent_username_and_rating} opponent_all doesn't have opponent username in json\n");
        cJSON_Delete(root);
        return;
    }
    if(opponent_rater == NULL){
        printf("{set_opponent_username_and_rating} opponent_all doesn't have opponent rating in json\n");
        cJSON_Delete(root);
        return;
    }
    char* opponent_actual_name = opponent_user->valuestring;
    uint32_t opponent_actual_rating = opponent_rater->valueint;

    int n = strlen(opponent_actual_name);
    for(int i = 0; i < n; i++){
        opponent_username[i] = opponent_actual_name[i];
    }
    sprintf(opponent_rating, "%ld", opponent_actual_rating);
    cJSON_Delete(root);
}

void set_user_country(char* username){
    char URL[100] = "https://lichess.org/api/user/";
    strcat(URL, username);
    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Authorization", bearer_token);
    const char* TAG = "LICHESS_COUNTRY";
    
    // GET
    esp_err_t err = esp_http_client_perform(client);
    ESP_LOGI(TAG, "Response data: %.*s", (int)esp_http_client_get_content_length(client), response_buf);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    bool me = false;
    if(strcmp(user_name, username) == 0){
        me = true;
    }

    char* json = response_buf;
    cJSON* root = cJSON_Parse(json);
    if(root == NULL){
        printf("{get_user_country} Root json is null");
        return;
    }
    cJSON* profiler = cJSON_GetObjectItem(root, "profile");
    if(profiler == NULL){
        printf("{get_user_country} Profile json is null");
        cJSON_Delete(root);
        return;
    }
    cJSON* countrY = cJSON_GetObjectItem(profiler, "country");
    if(countrY == NULL){
        printf("{get_user_country} %s didn't have country set\n", username);
        cJSON_Delete(root);
        char* countryna = "N/A";
        if(me){
            country[0] = 'N';
            country[1] = '/';
            country[2] = 'A';
        }
        else{
            opponent_country[0] = 'N';
            opponent_country[1] = '/';
            opponent_country[2] = 'A';
        }
    }
    char* COUNTRY = countrY->valuestring;
    int n = strlen(COUNTRY);
    for(int i = 0; i < n; i++){
        if(me){
            country[i] = COUNTRY[i];
        }
        else{
            opponent_country[i] = COUNTRY[i];
        }
    }
    cJSON_Delete(root);
}


void lichess_api_make_move(char user_move[]) {
    // TODO, remove this comment after
    // if (!logged_in) {
    //     return;
    // }
    xSemaphoreTake(xSemaphore_API, portMAX_DELAY);
    char URL[100] = "https://lichess.org/api/board/game/";
    char move[10] = "/move/";
    strcat(URL, GAME_ID);
    strcat(URL, move);
    strcat(URL, user_move);
    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    const char* TAG = "LICHESS_POST_MOVE";

    esp_err_t err = esp_http_client_perform(client);
    ESP_LOGI(TAG, "Response data: %.*s", (int)esp_http_client_get_content_length(client), response_buf);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    xSemaphoreGive(xSemaphore_API);
}

void lichess_api_stream_event() {
    xSemaphoreTake(xSemaphore_API, portMAX_DELAY);
    printf("LICHESS_API_STREAM_EVENT SEMAPHORE_API\n");
    if (!logged_in) {
        return;
    }

    static char stream_data[550] = {};
    static esp_http_client_config_t config_stream = {
        .url = "https://lichess.org/api/stream/event",
        .path = "/get",
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
        .event_handler = _http_event_handler,
        .user_data = stream_data,
    };

    client_stream = esp_http_client_init(&config_stream);
    esp_http_client_set_header(client_stream, AUTHORIZATION_HEADER, bearer_token);
    esp_http_client_set_method(client_stream, HTTP_METHOD_GET);
    esp_http_client_open(client_stream, 0);
    esp_http_client_fetch_headers(client_stream);

    printf("Stream event once Response code %d\n", esp_http_client_get_status_code(client_stream));
    esp_http_client_read(client_stream, stream_data, 550);
    printf("%s\n", stream_data);
    set_game_id(stream_data);
    set_color(stream_data);
    set_opponent_username_and_rating(stream_data);
    printf("My username: %s\n", user_name);
    printf("My rating %s\n", rating);
    printf("My country %s\n", country);

    printf("Opponent username: %s\n", opponent_username);
    printf("Opponent rating: %s\n", opponent_rating);
    printf("Opponent country %s\n", opponent_country);

    if(strcmp(getColor(), "white") == 0){
        scoreboard_Chess_Setup(user_name, opponent_username, country, opponent_country, rating, opponent_rating);
    }
    else{
        scoreboard_Chess_Setup(opponent_username, user_name, opponent_country, country, opponent_rating, rating);
    }
    
    printf("GAME ID: %s\n", GAME_ID);
    game_created = true;
    printf("Game created boolean is true\n");
    xSemaphoreGive(xSemaphore_API);
    lichess_api_stream_move_of_game();
}

<<<<<<< HEAD
void lichess_api_create_game(bool rated, uint8_t minutes, uint8_t increment, bool white) {
=======
void lichess_api_create_game(bool rated, uint8_t minutes, uint8_t increment) {
    xSemaphoreTake(xSemaphore_API, portMAX_DELAY);
    printf("Lichess create game HAS SEMAPHORE API\n");
>>>>>>> 4247ee6 (Make game seems to work, others in progress)
    if (!logged_in) {
        printf("Can't create game. Login not detected\n");
        return;
    }

    if(minutes <= 0){
        printf("ERROR, MINUTES MUST BE >= 1");
        return;
    }    
    char FULL_PARAMS[1000] = {};
    char URL[100] = "https://lichess.org/api/board/seek";
    rated ? strcat(FULL_PARAMS, "rated=true&") : strcat(FULL_PARAMS, "rated=false&");

    char fullMin[100] = "time=";

    int min_size = (int)((ceil(log10(minutes))+1)*sizeof(char));

    char min_as_string[min_size];
    sprintf(min_as_string, "%d", minutes);
    strcat(fullMin, min_as_string);
    strcat(FULL_PARAMS, fullMin);
    strcat(FULL_PARAMS, "&variant=standard");

    if(white == true){
        strcat(FULL_PARAMS, "&color=white");
    }
    else{
        strcat(FULL_PARAMS, "&color=black");
    }    
    
    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, FULL_PARAMS, strlen(FULL_PARAMS));

    const char* TAG = "LICHESS_CREATE_GAME";
    printf("Full params: Length %d\n%s\n", strlen(FULL_PARAMS), FULL_PARAMS);
    esp_err_t err = esp_http_client_perform(client);
    printf("Response code %d\n", esp_http_client_get_status_code(client));
    ESP_LOGI(TAG, "Response data: %.*s", (int)esp_http_client_get_content_length(client), response_buf);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    // TODO, Verify that this xSemaphoreGive is placed correctly
    xSemaphoreGive(xSemaphore_API);
    lichess_api_stream_event();
}

void lichess_api_get_email(void)
{
    xSemaphoreTake(xSemaphore_API, portMAX_DELAY);
    if (!logged_in) {
        return;
    }

    esp_http_client_set_url(client, "https://lichess.org/api/account/email");
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Authorization", bearer_token);
    const char* TAG = "LICHESS_EMAIL";
    
    // GET
    esp_err_t err = esp_http_client_perform(client);
    ESP_LOGI(TAG, "Response data: %.*s", (int)esp_http_client_get_content_length(client), response_buf);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    xSemaphoreGive(xSemaphore_API);
}

void lichess_api_get_account_info(void){
    const char* TAG = "LICHESS_ACCOUNT_INFO";
    
    printf("Inside lichess_get_account_info\n");
    esp_http_client_set_url(client, "https://lichess.org/api/account");
    esp_http_client_set_method(client, HTTP_METHOD_GET);    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char test_client_header[100] = {};
    esp_http_client_get_header(client, "Content-Type", test_client_header);
    esp_err_t err = esp_http_client_perform(client);
    // ESP_LOGI(TAG, "Response data: %.*s", (int)esp_http_client_get_content_length(client), response_buf);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    set_username(response_buf);
    printf("My username is %s\n", user_name);
    set_rating(response_buf);
    printf("My rating is %s\n", rating);
    xSemaphoreGive(xSemaphore_API);
}

void lichess_api_init_client(void) {
    xSemaphore_API = xSemaphoreCreateBinary();
    xSemaphore_DataTransfer = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore_DataTransfer);
    static const char* TAG = "LICHESS_INIT";
    client = esp_http_client_init(&config);
    logged_in = false;
    ESP_LOGI(TAG, "Complete");
}

// TODO, REPLACE THIS BACK
void lichess_api_login(const char* token, const uint16_t token_len) {
    strcpy(bearer_token, "Bearer ");
    // strncat(bearer_token, token, token_len);    
    printf("Inside lichess_api_login\n");
    const char* replace = "lip_EFm5BNTU3XP4nvczcv1a";
    size_t len = strlen(replace) + 1;
    strncat(bearer_token, replace, len);
    printf("Bearer token %s\n", bearer_token);
    esp_http_client_set_header(client, "Authorization", bearer_token);
    lichess_api_get_account_info();
    logged_in = true;
}

void lichess_api_logout() {
    esp_http_client_delete_header(client, AUTHORIZATION_HEADER);
    logged_in = false;
}

bool lichess_api_is_logged_in() {
    return logged_in;
}

void lichess_api_resign_game(){
    // if(strlen(GAME_ID) == 0){
    //     printf("lichess_api_resign_game. GAME ID IS NOT SET\n");
    //     return;
    // }
    printf("Inside lichess_api_resign_game\n");
    const char* TAG = "LICHESS_RESIGN_GAME";
    
    // char TEST_GAME[100] = "w6jbDnGg";
    char URL[100] = "https://lichess.org/api/board/game/";
    strncat(URL, GAME_ID, strlen(GAME_ID));
    char last[10] = "/resign";
    strncat(URL, last, strlen(last));


    xSemaphoreTake(xSemaphore_DataTransfer, portMAX_DELAY);
    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST); 
    esp_http_client_set_header(client, "Content-Type", "text/plain");
    esp_err_t err = esp_http_client_perform(client);
    xSemaphoreGive(xSemaphore_DataTransfer);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
}

void lichess_api_handle_draw(){
    xSemaphoreTake(xSemaphore_API, portMAX_DELAY);
    
    printf("Inside lichess_api_handle_draw\n");
    const char* TAG = "LICHESS_HANDLE_DRAW";
    char URL[100] = "https://lichess.org/api/board/game/";
    strcat(URL, GAME_ID);
<<<<<<< HEAD
    strcat(URL, "/draw/yes");
=======
    strcat(URL, "/draw/");
    strcat(URL, "yes");
>>>>>>> 4247ee6 (Make game seems to work, others in progress)

    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_err_t err = esp_http_client_perform(client);
    xSemaphoreGive(xSemaphore_API);
}

void lichess_api_stream_move_of_game(void *pvParameters) {
    xSemaphoreTake(xSemaphore_API, portMAX_DELAY);
    printf("Inside Lichess_api_stream_move_of_game\n");
    static const char* TAG = "LICHESS STREAM MOVE";

    // TODO, uncomment this after done
    // if (!logged_in) {
    //     return;
    // }
    //

    if(strlen(GAME_ID) == 0){
        printf("ERROR, GAME ID NOT DETECTED\n");
        return;
    }

    char URL[100] = "https://lichess.org/api/board/game/stream/";
    strcat(URL, GAME_ID);
    printf("URL %s\n", URL);
    esp_http_client_config_t config_stream = {
        .url = URL,
        .path = "/get",
        .transport_type = HTTP_TRANSPORT_OVER_TCP
    };
    esp_http_client_handle_t client_stream = esp_http_client_init(&config_stream);    
    esp_http_client_set_header(client_stream, "Authorization", bearer_token);
    esp_http_client_set_method(client_stream, HTTP_METHOD_GET);
    esp_err_t err;
    if ((err = esp_http_client_open(client_stream, 0)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return;
    }

    esp_http_client_fetch_headers(client_stream);
    want_moves = true;
    while(1){ 
        if(!wifi_is_connected()){
            ESP_LOGE(TAG, "WIFI DISCONNECTED");
            break;
        }
        char stream_data[1500] = {};
        int curr_stream_data_len = 0;
        char buffer[1000] = {};
        while(buffer[0] != '\n'){
            xSemaphoreTake(xSemaphore_DataTransfer, portMAX_DELAY);
            esp_http_client_read(client_stream, buffer, 1);
            stream_data[curr_stream_data_len] = buffer[0];
            curr_stream_data_len += 1;
            xSemaphoreGive(xSemaphore_DataTransfer);
        }
        if(strlen(stream_data) > 1){
            printf("Stream is %s\n", stream_data);
        }
        stream_data[strlen(stream_data)] = 0;
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
                    esp_http_client_get_status_code(client_stream),
                    esp_http_client_get_content_length(client_stream));
        } else {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }

        if(strlen(stream_data) > 1){
            char* result = check_result_of_game(stream_data);
            if(strcmp(result, "GAME IN PROGRESS") == 0){
                printf("Game in progress\n");
            }
            else if(strcmp(result, "1/2 - 1/2. Game drawn") == 0){
                want_moves = false;
                char p1[5] = "1/2";
                char p2[5] = "1/2";
                scoreboard_WinUpdate(p1, p2);
                printf("Game Ended: %s\n", result);
                game_created = false;
                break;
            }
            else if(strcmp(result, "0-1 (Black wins)") == 0){
                    want_moves = false;                    
                    char p1[5] = "0";
                    char p2[5] = "1";
                    scoreboard_WinUpdate(p1, p2);
                    printf("Game Ended: Black wins. 0-1\n");
                    game_created = false;
                    break;
            }
            else if(strcmp(result, "1-0 (White wins)") == 0){
                    want_moves = false;
                    char p1[5] = "1";
                    char p2[5] = "0";
                    scoreboard_WinUpdate(p1, p2);
                    printf("Game Ended: White wins. 1-0\n");
                    game_created = false;
                    break;
            }
            else{
                // Just continue the game
            }
            set_last_move_played_by_opponent(stream_data);
            set_clock_time(stream_data);
            printf("Official last move: %s\n", get_last_move_played_by_opponent());
            printf("White has %lu time\n", white_time);
            printf("Black has %lu time\n", black_time);
            if(strcmp(getColor(), "white") == 0){ 
                portDISABLE_INTERRUPTS();
                GraphicLCD_DispClock(white_time, true);
                GraphicLCD_DispClock(black_time, false);
                portENABLE_INTERRUPTS();
            }
            else{             
                portDISABLE_INTERRUPTS();
                GraphicLCD_DispClock(white_time, false);
                GraphicLCD_DispClock(black_time, true);
                portENABLE_INTERRUPTS();
            }
            our_turn = our_turn ^ 1;           
        }
    }
    xSemaphoreGive(xSemaphore_API);
}
