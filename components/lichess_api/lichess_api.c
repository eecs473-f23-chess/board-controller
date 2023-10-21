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
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            // if (!esp_http_client_is_chunked_response(evt->client)) {
            //     // If user_data buffer is configured, copy the response into the buffer
            //     sleep(5);
            //     int copy_len = 0;
            //     if (evt->user_data) {
            //         // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
            //         copy_len = MIN(evt->data_len, MAX_HTTP_OUTPUT_BUFFER);
            //         if (copy_len) {
            //             memcpy(evt->user_data, evt->data, copy_len);
            //         }
            //     } else {
            //         ESP_LOGE(TAG, "Expected user data buffer");
            //     }
            // }
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

void lichess_api_login(const char* token, const uint16_t token_len) {
    strcpy(bearer_token, "Bearer ");
    strncat(bearer_token, token, token_len);
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
    cJSON *game = cJSON_GetObjectItem(root, "game");
    cJSON *moves = cJSON_GetObjectItem(game, "lastMove");
    char* full_moves = moves->valuestring;
    int n = strlen(full_moves);
    for(int i = n-4; i < n; i++){
        last_move_played_by_opponent[i-n+4] = full_moves[i];
    }
}


char* lichess_api_get_username() {
    return user_name;
}
/*
    game_id: ID of the game being played
*/
void lichess_api_stream_move_of_game() {
    if (!logged_in) {
        return;
    }

    if(strlen(GAME_ID) == 0){
        printf("ERROR, GAME ID NOT DETECTED\n");
        return;
    }
    char URL[100] = "https://lichess.org/api/board/game/stream/";
    strcat(URL, GAME_ID);
    esp_http_client_set_url(client, URL);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    const char* TAG = "LICHESS_GAME_STATE";

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

/*
    game_id: ID of the game being played
    move: a FOUR character character array that contains the source and destination square (a1a2)
*/
void lichess_api_make_move(char user_move[]) {
    if (!logged_in) {
        return;
    }

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
    int read_value = esp_http_client_read(client_stream, stream_data, 550);
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
