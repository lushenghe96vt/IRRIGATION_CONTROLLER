#pragma once
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <string.h>

// initialize Wifi for both boards

void wifi_init(const char *ssid, const char *password);