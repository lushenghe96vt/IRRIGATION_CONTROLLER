#include "http_server.h"
#include "esp_http_server.h"

#define TAG "HTTP_SERVER"

#define num_sensors 2 // number of sensors used, can be changed as needed
static MoistureSensor sensors[num_sensors]; // number of sensors used

esp_err_t moisture_post_handler(httpd_req_t * req) {
    // getting json data
    char buf[128]; 
    int ret = httpd_req_recv(req, buf, MIN(req->content_len, sizeof(buf) - 1));
    if (ret <= 0){ 
        ESP_LOGE(TAG, "No data to post");
        return ESP_FAIL;
    }
    buf[ret] = '\0'; 
    ESP_LOGI(TAG, "Received JSON: %s", buf);

    // parsing JSON
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    // extract fields
    cJSON *sensor_id = cJSON_GetObjectItem(root, "id");
    if (!cJSON_IsNumber(sensor_id) || sensor_id->valueint < 1 || sensor_id->valueint > num_sensors) {
        ESP_LOGE(TAG, "Sensor ID is not a number");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid sensor ID");
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    int id = sensor_id->valueint;
    cJSON *raw_level = cJSON_GetObjectItem(root, "raw_level");
    cJSON *dryness_level = cJSON_GetObjectItem(root, "dryness_level");

    // error handling for incorrect types
    if (!cJSON_IsNumber(raw_level) || !cJSON_IsNumber(dryness_level)) {
        ESP_LOGE(TAG, "Invalid JSON fields");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    // update sensor
    sensors[id - 1].raw_level = raw_level->valueint;
    sensors[id - 1].dryness_level = (float)dryness_level->valuedouble;

    ESP_LOGI(TAG, "Updated sensor[%d]: raw=%d, moist=%.2f", id - 1, sensors[id - 1].raw_level, sensors[id - 1].dryness_level);
    cJSON_Delete(root);
    httpd_resp_send(req, "OK", 2);
    return ESP_OK;
}

void start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t moisture_uri = {
            .uri = "/moisture", // posts to ip_address/moisture
            .method = HTTP_POST,
            .handler = moisture_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &moisture_uri);
    }
}