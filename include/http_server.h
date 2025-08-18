#pragma once
#include <sys/param.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "sensor.h"
#include "SensorContext.h"

esp_err_t start_http_server(SensorContext *ctx);
void http_ws_broadcast_snapshot(SensorContext *ctx);