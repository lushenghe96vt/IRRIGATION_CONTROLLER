#include <string.h>
#include <sys/param.h>
#include <stdlib.h>

#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#define TAG "HTTP_SERVER"

// handles provided in main
extern QueueHandle_t g_readings_q;
extern SemaphoreHandle_t g_sensors_mutex;

// matches struct in main
typedef struct {
    int id;
    int raw_level;
    float dryness_level;
    uint32_t ts_ms;
} reading_t;

// manaul override for valve control
extern void control_set_manual(bool on, uint32_t seconds);
extern void control_set_auto(void);

// websocket client management
#ifndef MAX_WS_CLIENTS
#define MAX_WS_CLIENTS 8
#endif

static httpd_handle_t s_server = NULL;
static int s_ws_fds[MAX_WS_CLIENTS];
static size_t s_ws_n = 0;
static SemaphoreHandle_t s_ws_lock = NULL;

static inline void ws_lock(void)   { if (s_ws_lock) xSemaphoreTake(s_ws_lock, portMAX_DELAY); }
static inline void ws_unlock(void) { if (s_ws_lock) xSemaphoreGive(s_ws_lock); }

static void ws_add_fd(int fd) {
    ws_lock();
    for (size_t i = 0; i < s_ws_n; ++i) if (s_ws_fds[i] == fd) { ws_unlock(); return; }
    if (s_ws_n < MAX_WS_CLIENTS) s_ws_fds[s_ws_n++] = fd;
    ws_unlock();
}

static void ws_remove_fd(int fd) {
    ws_lock();
    for (size_t i = 0; i < s_ws_n; ++i) {
        if (s_ws_fds[i] == fd) {
            s_ws_fds[i] = s_ws_fds[s_ws_n - 1];
            s_ws_n--;
            break;
        }
    }
    ws_unlock();
}

/* Build snapshot JSON from ctx into out buffer. Returns length written. */
static size_t build_snapshot_json(SensorContext *ctx, char *out, size_t out_sz) {
    size_t pos = 0;
    if (!out_sz) return 0;
    out[pos++] = '[';

    xSemaphoreTake(g_sensors_mutex, portMAX_DELAY);
    for (int i = 0; i < ctx->num_sensors; ++i) {
        int n = snprintf(out + pos, (pos < out_sz ? out_sz - pos : 0),
                         "%s{\"id\":%d,\"raw_level\":%d,\"dryness_level\":%.3f}",
                         (i ? "," : ""),
                         ctx->sensors[i].id,
                         ctx->sensors[i].raw_level,
                         ctx->sensors[i].dryness_level);
        if (n < 0) n = 0;
        pos += (size_t)n;
        if (pos >= out_sz) break;
    }
    xSemaphoreGive(g_sensors_mutex);

    if (pos + 2 <= out_sz) { out[pos++] = ']'; out[pos] = '\0'; }
    else { out[out_sz - 2] = ']'; out[out_sz - 1] = '\0'; pos = out_sz - 1; }
    return pos;
}

/* Public: broadcast current ctx snapshot to all WS clients. Safe to call from tasks. */
void http_ws_broadcast_snapshot(SensorContext *ctx) {
    if (!s_server || !s_ws_n) return;

    char snap[1024];
    build_snapshot_json(ctx, snap, sizeof(snap));

    httpd_ws_frame_t frame = {
        .type    = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)snap,
        .len     = strlen(snap)
    };

    /* Send asynchronously using server handle + socket fd */
    ws_lock();
    /* Make a copy of the fds so we can modify list if a send fails */
    int fds[MAX_WS_CLIENTS]; size_t n = s_ws_n;
    for (size_t i = 0; i < n; ++i) fds[i] = s_ws_fds[i];
    ws_unlock();

    for (size_t i = 0; i < n; ++i) {
        esp_err_t r = httpd_ws_send_frame_async(s_server, fds[i], &frame);
        if (r != ESP_OK) {
            ESP_LOGW(TAG, "ws send failed fd=%d err=%d; removing client", fds[i], (int)r);
            ws_remove_fd(fds[i]);
        }
    }
}

// Handlers
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
        "<p><a href='/data'>/data</a> (JSON)</p>"
        "<label>Seconds: <input id='sec' type='number' value='15' min='1' max='3600'></label>"
        "<div style='margin-top:8px'>"
        "<button onclick='send(true)'>Manual ON</button> "
        "<button onclick='send(false)'>Manual OFF</button> "
        "<button onclick='autoMode()'>Auto</button>"
        "</div>"
        "<pre id='out'></pre>"
        "<script>"
        "const out=document.getElementById('out');"
        "function send(on){const s=+document.getElementById(\"sec\").value||15;"
        " fetch(\"/override\",{method:\"POST\",headers:{\"Content-Type\":\"application/json\"},"
        " body:JSON.stringify({on:on,seconds:s})}).then(r=>r.text()).then(t=>out.textContent=t);}"
        "function autoMode(){fetch(\"/override\",{method:\"POST\",headers:{\"Content-Type\":\"application/json\"},"
        " body:JSON.stringify({mode:\"auto\"})}).then(r=>r.text()).then(t=>out.textContent=t);}"
        "</script>";
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


// websocket route

static esp_err_t ws_handler(httpd_req_t *req) {
    SensorContext *ctx = (SensorContext *)req->user_ctx;

    if (req->method == HTTP_GET) {
        /* handshake done; remember client and push one snapshot */
        ws_add_fd(httpd_req_to_sockfd(req));

        char snap[1024];
        build_snapshot_json(ctx, snap, sizeof(snap));
        httpd_ws_frame_t frame = {
            .type    = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t *)snap,
            .len     = strlen(snap)
        };
        return httpd_ws_send_frame(req, &frame);
    }

    /* For any incoming message, just reply with a fresh snapshot. */
    httpd_ws_frame_t in = { .type = HTTPD_WS_TYPE_TEXT };
    esp_err_t ret = httpd_ws_recv_frame(req, &in, 0); // get length
    if (ret != ESP_OK) return ret;

    uint8_t *buf = NULL;
    if (in.len) {
        buf = (uint8_t *)malloc(in.len + 1);
        if (!buf) return ESP_ERR_NO_MEM;
        in.payload = buf;
        ret = httpd_ws_recv_frame(req, &in, in.len);
        if (ret != ESP_OK) { free(buf); return ret; }
        buf[in.len] = '\0';
    }

    char snap[1024];
    build_snapshot_json(ctx, snap, sizeof(snap));
    httpd_ws_frame_t out = {
        .type    = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)snap,
        .len     = strlen(snap)
    };
    ret = httpd_ws_send_frame(req, &out);

    if (buf) free(buf);
    return ret;
}

static esp_err_t override_post_handler(httpd_req_t *req) {
    char buf[128];
    int len = httpd_req_recv(req, buf, MIN(req->content_len, (int)sizeof(buf) - 1));
    if (len <= 0) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");
    buf[len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad json");

    cJSON *mode = cJSON_GetObjectItem(root, "mode");
    if (cJSON_IsString(mode) && strcmp(mode->valuestring, "auto") == 0) {
        control_set_auto();
        cJSON_Delete(root);
        return httpd_resp_sendstr(req, "OK");
    }

    cJSON *on      = cJSON_GetObjectItem(root, "on");
    cJSON *seconds = cJSON_GetObjectItem(root, "seconds");
    if (!cJSON_IsBool(on) || !cJSON_IsNumber(seconds)) {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "need {on:bool,seconds:number} or {mode:\"auto\"}");
    }

    int sec = seconds->valueint;
    if (sec < 1) sec = 1;
    if (sec > 3600) sec = 3600; // clamp to 1 hour max

    control_set_manual(cJSON_IsTrue(on), (uint32_t)sec);
    cJSON_Delete(root);
    return httpd_resp_sendstr(req, "OK");
}

// bootstrap HTTP server
esp_err_t start_http_server(SensorContext *ctx) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) return ret;

    s_server = server;
    if (!s_ws_lock) s_ws_lock = xSemaphoreCreateMutex();

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

    httpd_uri_t ws_uri = {
        .uri          = "/ws",
        .method       = HTTP_GET,
        .handler      = ws_handler,
        .user_ctx     = ctx,
        .is_websocket = true
    };
    httpd_register_uri_handler(server, &ws_uri);

    httpd_uri_t override_uri = {
    .uri      = "/override",
    .method   = HTTP_POST,
    .handler  = override_post_handler,
    .user_ctx = ctx
    };
    httpd_register_uri_handler(server, &override_uri);
    
    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}