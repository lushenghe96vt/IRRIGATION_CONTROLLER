#include "http_server.h"
#include "esp_http_server.h"

#define TAG "HTTP_SERVER"

esp_err_t moisture_post_handler(httpd_req_t * req) {
    SensorContext *ctx = (SensorContext *)req->user_ctx;
    MoistureSensor *sensors = ctx->sensors;
    int num_sensors = ctx->num_sensors;

    // getting json data
    char buf[256]; 
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

esp_err_t web_index_handler(httpd_req_t *req)
{
    const char *html_response =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>ESP32 Moisture Dashboard</title></head>"
        "<body>"
        "<h1>ESP32 Moisture Sensor Data</h1>"
        "<pre id=\"data\">Loading moisture data...</pre>"
        "<script>"
        "function fetchData() {"
        "  fetch('/data')"
        "    .then(response => response.json())"
        "    .then(data => {"
        "      let text = '';"
        "      data.forEach(sensor => {"
        "        text += `Sensor ${sensor.id} - Raw: ${sensor.raw_level}, Dryness Level: ${sensor.dryness_level}\\n`;"
        "      });"
        "      document.getElementById('data').textContent = text;"
        "    })"
        "    .catch(err => console.error('Fetch error:', err));"
        "}"
        "setInterval(fetchData, 1000);"
        "fetchData();"
        "</script>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
}

esp_err_t web_data_handler(httpd_req_t *req) {
    SensorContext *ctx = (SensorContext *)req->user_ctx;

    // Assuming you have NUM_SENSORS sensors
    static char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
        "[{\"id\":%d,\"raw_level\":%d,\"dryness_level\":%.2f},"
        "{\"id\":%d,\"raw_level\":%d,\"dryness_level\":%.2f}]",
        ctx->sensors[0].id, ctx->sensors[0].raw_level, ctx->sensors[0].dryness_level,
        ctx->sensors[1].id, ctx->sensors[1].raw_level, ctx->sensors[1].dryness_level
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_buf, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t start_http_server(SensorContext *ctx) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    httpd_uri_t moisture_uri = {
        .uri = "/moisture", // posts to ip_address/moisture
        .method = HTTP_POST,
        .handler = moisture_post_handler,
        .user_ctx = ctx
     };
    httpd_register_uri_handler(server, &moisture_uri);

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = web_index_handler,
        .user_ctx  = ctx
    };
    httpd_register_uri_handler(server, &index_uri);

    httpd_uri_t data_endpoint = {
    .uri       = "/data",
    .method    = HTTP_GET,
    .handler   = web_data_handler,
    .user_ctx  = ctx
    };
    httpd_register_uri_handler(server, &data_endpoint);
    
    return ESP_OK;
}