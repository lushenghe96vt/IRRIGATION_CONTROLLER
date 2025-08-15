#include <string.h>
#include <sys/param.h>

#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define TAG "HTTP_SERVER"

// handles provided in main
extern QueueHandle_t     g_readings_q;
extern SemaphoreHandle_t g_sensors_mutex;

// matches struct in main
typedef struct {
    int       id;
    int       raw_level;
    float     dryness_level;
    uint32_t  ts_ms;
} reading_t;

esp_err_t moisture_post_handler(httpd_req_t * req) {
    SensorContext *ctx = (SensorContext *)req->user_ctx;

    // getting json data
    char buf[256];
    int len = httpd_req_recv(req, buf, MIN(req->content_len, (int)sizeof(buf) - 1));
    if (len <= 0) {
        ESP_LOGE(TAG, "Empty POST body");
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
    }
    buf[len] = '\0';

    // parsing JSON
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad json");
    }

    // extract fields
    cJSON *sensor_id = cJSON_GetObjectItem(root, "id"); // must match schemes in sensor.c
    cJSON *raw_level = cJSON_GetObjectItem(root, "raw_level");
    cJSON *dryness_level = cJSON_GetObjectItem(root, "dryness_level");

    if (!cJSON_IsNumber(sensor_id) ||
        sensor_id->valueint < 1 || sensor_id->valueint > ctx->num_sensors ||
        !cJSON_IsNumber(raw_level) || !cJSON_IsNumber(dryness_level)) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid fields");
    }

    // updating sensor through queue
    reading_t r = {
        .id            = sensor_id->valueint,
        .raw_level     = raw_level->valueint,
        .dryness_level = (float)dryness_level->valuedouble,
        .ts_ms         = (uint32_t)esp_log_timestamp()
    };

    if (xQueueSend(g_readings_q, &r, 0) != pdTRUE) {
        ESP_LOGW(TAG, "readings_q full; dropping"); // queue is full, but still respond 200
    }

    cJSON_Delete(root);
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t web_index_handler(httpd_req_t *req) {
    static const char html[] =
        "<!doctype html><meta charset=utf-8>"
        "<h1>Irrigation Controller</h1>"
        "<ul>"
        "<li><a href='/data'>/data</a> – current sensors JSON</li>"
        "</ul>"
        "<p>Nodes POST JSON to <code>/moisture</code> like "
        "<code>{\"id\":1,\"raw_level\":1234,\"dryness_level\":0.42}</code>.</p>";
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t web_data_handler(httpd_req_t *req) {
    SensorContext *ctx = (SensorContext *)req->user_ctx;
    httpd_resp_set_type(req, "application/json");

    httpd_resp_sendstr_chunk(req, "[");
    xSemaphoreTake(g_sensors_mutex, portMAX_DELAY);
    for (int i = 0; i < ctx->num_sensors; ++i) {
        char item[128];
        snprintf(item, sizeof(item),
                 "%s{\"id\":%d,\"raw_level\":%d,\"dryness_level\":%.3f}",
                 (i == 0 ? "" : ","),
                 ctx->sensors[i].id,
                 ctx->sensors[i].raw_level,
                 ctx->sensors[i].dryness_level);
        httpd_resp_sendstr_chunk(req, item);
    }
    xSemaphoreGive(g_sensors_mutex);
    httpd_resp_sendstr_chunk(req, "]");
    return httpd_resp_sendstr_chunk(req, NULL);
}

esp_err_t start_http_server(SensorContext *ctx) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) return ret;

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

    httpd_uri_t data_uri = {
    .uri       = "/data",
    .method    = HTTP_GET,
    .handler   = web_data_handler,
    .user_ctx  = ctx
    };
    httpd_register_uri_handler(server, &data_uri);
    
    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}