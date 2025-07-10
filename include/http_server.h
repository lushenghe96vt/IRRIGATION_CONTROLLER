#pragma once
#include <sys/param.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include "sensor.h"

void start_http_server(void);