#include "lichess_api.h"

#include <esp_http_client.h>
#include <esp_log.h>
#include <math.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <string.h> 
#include <cJSON.h>

#define MAX_HTTP_OUTPUT_BUFFER  4096
#define AUTHORIZATION_HEADER    "Authorization"
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

static char GAME_ID[100] = {};
static char last_move_played_by_opponent[5] = {};
static char response_buf[3000] = {};
static char color[10] = {};
static char user_name[100] = {};
static char bearer_token[64] = {};
static bool logged_in;
static bool move_update = false;
static bool want_moves = false;
static uint32_t white_time = -1;
static uint32_t black_time = -1;
static esp_http_client_handle_t client;
static esp_http_client_handle_t client_stream;


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
                printf("Data not chunked\n");
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
        printf("Error parsing JSON.\n");
        return;
    }
    cJSON *user = cJSON_GetObjectItem(root, "username");
    for(int i = 0; i < strlen(user->valuestring); i++){
        user_name[i] = (user->valuestring)[i];
    }  
    cJSON_Delete(root);
}

void lichess_api_get_account_info(void){
    const char* TAG = "LICHESS_ACCOUNT_INFO";
    esp_http_client_set_url(client, "https://lichess.org/api/account");
    esp_http_client_set_method(client, HTTP_METHOD_GET);    
    esp_http_client_set_header(client, "Content-Type", "application/json");
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

}

void lichess_api_init_client(void) {
    static const char* TAG = "LICHESS_INIT";
    client = esp_http_client_init(&config);
    logged_in = false;
    ESP_LOGI(TAG, "Complete");
}

// TODO, REPLACE THIS BACK
void lichess_api_login(const char* token, const uint16_t token_len) {
    strcpy(bearer_token, "Bearer ");
    // strncat(bearer_token, token, token_len);
    const char* replace = "lip_w2grwolW4y407Qjj0DWz";
    size_t len = strlen(replace) + 1;
    strncat(bearer_token, replace, len);
    printf("Bearer token %s\n", bearer_token);
    esp_http_client_set_header(client, "Authorization", bearer_token);
    logged_in = true;
}

void lichess_api_logout() {
    esp_http_client_delete_header(client, AUTHORIZATION_HEADER);
    logged_in = false;
}

bool lichess_api_is_logged_in() {
    return logged_in;
}

// TODO, Make it one function so we can parse it once. 
void set_game_id(const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);    
    if (root == NULL) {
        printf("Error parsing JSON.\n");
        return;
    }
    cJSON *game = cJSON_GetObjectItem(root, "game");
    cJSON *gameId = cJSON_GetObjectItem(game, "gameId");
    for(int i = 0; i < strlen(gameId->valuestring); i++){
        GAME_ID[i] = (gameId->valuestring)[i];
    }  
    printf("Game ID %s\n", GAME_ID);
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
        printf("ERROR PARSING JSON\n");
        return;
    }
    // TODO, based off stream board game state
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
        return;
    }
    char* full_moves = moves->valuestring;
    int n = strlen(full_moves);
    for(int i = n-4; i < n; i++){
        last_move_played_by_opponent[i-n+4] = full_moves[i];
    }
}

char* check_result_of_game(char *json){
    char* game_in_progress = "GAME IN PROGRESS";
    char* white_resigned = "0-1 (Black wins)";
    char* black_resigned = "1-0 (White wins)";
    cJSON *root = cJSON_Parse(json);
    if(root == NULL){
        printf("check_result_of_game ERROR PARSING JSON\n");
        return NULL;
    }
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if(type == NULL){
        printf("Type error in check result\n");
        return NULL;
    }
    cJSON *status = NULL;
    char* game_status = NULL;
    if(strcmp(type->valuestring, "gameFull") == 0){
        status = cJSON_GetObjectItem(root, "state");
        if(status == NULL){
            printf("Status is null in get result\n");
        }
    }
    else if(strcmp(type->valuestring, "gameState") == 0) {
        status = cJSON_GetObjectItem(root, "status");
        cJSON* draw_potentialb = cJSON_GetObjectItem(root, "bdraw");
        if(draw_potentialb != NULL){
            printf("Black has offered a draw!\n");
        }
        cJSON* draw_potentialw = cJSON_GetObjectItem(root, "wdraw");
        if(draw_potentialw != NULL){
            printf("White has offered a draw!\n");
        }
        if(status == NULL){
            printf("Status is null in get result\n");
        }
        game_status = status->valuestring;
    }
    else if(strcmp(type->valuestring, "chatLine") == 0){
        cJSON* text = cJSON_GetObjectItem(root, "text");
        char* firstOption = "Draw offer accepted";
        if(strcmp(firstOption, text->valuestring) == 0){
            return "1/2 - 1/2. Game drawn";
        }
        else{
            return text->valuestring;
        }
    }
    if(strcmp(type->valuestring, "gameFull") == 0){
        cJSON* real_status = cJSON_GetObjectItem(status, "status");
        game_status = real_status->valuestring;
    }

    if(game_status == NULL){
        printf("Game status is null\n");
        return NULL;
    }
    // printf("Game status value %s\n", game_status);
    if(strcmp(game_status, "started") == 0){
        // TODO, integrate draw offers
        return game_in_progress;
    }
    else if(strcmp(game_status, "resign") == 0 || strcmp(game_status, "mate") == 0){
        cJSON *winner = cJSON_GetObjectItem(root, "winner");
        char* winner_str = winner->valuestring;
        if(strcmp(winner_str, "white") == 0){
            return black_resigned;
        }
        if(strcmp(winner_str, "black") == 0){
            return white_resigned;
        }
    }
    return "ERROR IN check_result_of_game";
    
}

char* get_last_move_played_by_opponent(){
    return last_move_played_by_opponent;
}

// TODO, convert seconds to HOUR: MINUTE: SECOND format
void set_clock_time(char *json){
    cJSON *root = cJSON_Parse(json);
    if(root == NULL){
        printf("ERROR in set_clock_time\n");
        return;
    }
    // TODO, based off stream board game state
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
        return;
    }
}


char* lichess_api_get_username() {
    return user_name;
}
/*
    game_id: ID of the game being played
*/
void lichess_api_stream_move_of_game(void *pvParameters) {
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
        char stream_data[1500] = {};
        char buffer[1000] = {};
        while(buffer[0] != '\n'){
            esp_http_client_read(client_stream, buffer, 1);
            stream_data[strlen(stream_data)] = buffer[0];
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
                printf("Game Ended: %s\n", result);
                break;
            }
            else if(strcmp(result, "0-1 (Black wins)") == 0){
                    want_moves = false;
                    printf("Game Ended: Black wins. 0-1\n");
                    break;
            }
            else if(strcmp(result, "1-0 (White wins)") == 0){
                    want_moves = false;
                    printf("Game Ended: White wins. 1-0\n");
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
        }
    }
    vTaskDelete(NULL);
}

bool get_opponent_move_update(){
    return move_update;
}

void reset_opponent_move_update(){
    move_update = false;
}

/*
    game_id: ID of the game being played
    move: a FOUR character character array that contains the source and destination square (a1a2)
*/
void lichess_api_make_move(char user_move[]) {
    // TODO, remove this comment after
    // if (!logged_in) {
    //     return;
    // }

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
}

/*
    "Stream the events reaching a lichess user in real time as ndjson.
    An empty line is sent every 6 seconds for keep alive purposes." - Lichess Documentation
*/
void lichess_api_stream_event() {
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

    printf("Response code %d\n", esp_http_client_get_status_code(client_stream));
    esp_http_client_read(client_stream, stream_data, 550);
    printf("%s\n", stream_data);
    set_game_id(stream_data);
    set_color(stream_data);
    printf("GAME ID: %s\n", GAME_ID);
}

void lichess_api_create_game(bool rated, uint8_t minutes, uint8_t increment) {
    if (!logged_in) {
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
    // char fullInc[100] = "&increment=";

    int min_size = (int)((ceil(log10(minutes))+1)*sizeof(char));
    // int inc_size = 1;
    // if(increment >= 10){
    //    inc_size = (int)((ceil(log10(increment))+1)*sizeof(char));
    // }

    char min_as_string[min_size];
    // char inc_as_string[inc_size];

    sprintf(min_as_string, "%d", minutes);
    // sprintf(inc_as_string, "%d", increment);

    strcat(fullMin, min_as_string);
    // strcat(fullInc, inc_as_string);

    strcat(FULL_PARAMS, fullMin);
    // strcat(FULL_PARAMS, fullInc);


    strcat(FULL_PARAMS, "&variant=standard");
    
    // printf("%s\n", FULL_PARAMS);
    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, FULL_PARAMS, strlen(FULL_PARAMS));
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    

    const char* TAG = "LICHESS_CREATE_GAME";
    printf("Full params length: %d\n", strlen(FULL_PARAMS));
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

    lichess_api_stream_event();
}

void lichess_api_get_email(void)
{
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
}

