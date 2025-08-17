#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "sensor.h"
#include "http_server.h"
#include "wifi_manager.h"
#include "SensorContext.h"

#define RELAY_OUT_D2 2                  // relay power pin  
#define MOISTURE_OUT_D4 4               // sensor power pin
#define MOISTURE_IN_D32 ADC1_CHANNEL_4  // GPIO32 = ADC1_CHANNEL_4

#define NUM_SENSORS 1 // should be 2 ----------------------change later
#define SENSOR_ID_1 1
#define SENSOR_ID_2 2

#define STK_DEFAULT_WORDS 4096  // stack size in words for tasks, set to 16kb
#define PRIO_HIGH 5             // high priority for creating tasks
#define PRIO_LOW 3              // low priority for creating tasks

// payload passed passed to and from queue
typedef struct {
    int id;
    int raw_level;
    float dryness_level;
    uint32_t ts_ms;
} reading_t;

static MoistureSensor sensors[NUM_SENSORS];

SensorContext ctx = {
    .sensors = sensors,
    .num_sensors = NUM_SENSORS
};

// rtos objects
QueueHandle_t g_readings_q = NULL;
SemaphoreHandle_t g_sensors_mutex = NULL;

// valve control
static const char *TAG = "APP";
static float g_thresh_low = 0.25f; // open when average dips below 10%
static float g_thresh_high = 0.45f; // open when average rises above 25% -----------experimental values-------------
static bool  g_valve_open = false;

static inline void valve_set(bool open){
    gpio_set_level(RELAY_OUT_D2, open ? 1 : 0);
    g_valve_open = open;
    ESP_LOGI(TAG, "Valve -> %s", open ? "OPEN" : "CLOSED");
}

// ingest task
static void ingest_task(void *arg) {
    reading_t r;
    while (1) {
        if (xQueueReceive(g_readings_q, &r, portMAX_DELAY) == pdTRUE) {
            if (r.id >= 1 && r.id <= ctx.num_sensors) {
                xSemaphoreTake(g_sensors_mutex, portMAX_DELAY);
                MoistureSensor *s = &ctx.sensors[r.id - 1];
                s->raw_level = r.raw_level;
                s->dryness_level = r.dryness_level;
                xSemaphoreGive(g_sensors_mutex);
            }
        }
    }
}

// control task
static void control_task(void *arg) {
    gpio_set_direction(RELAY_OUT_D2, GPIO_MODE_OUTPUT);
    valve_set(false);

    const TickType_t period = pdMS_TO_TICKS(5000); // 5 hz control loop
    while (1) {
        vTaskDelay(period);

        // Take the most conservative view: the driest sensor governs.
        float min_dryness = 1.0f;
        xSemaphoreTake(g_sensors_mutex, portMAX_DELAY);
        for (int i = 0; i < ctx.num_sensors; ++i) {
            if (ctx.sensors[i].dryness_level < min_dryness) {
                min_dryness = ctx.sensors[i].dryness_level;
            }
        }
        xSemaphoreGive(g_sensors_mutex);

        if (g_valve_open && min_dryness < g_thresh_low)  valve_set(false);
        if (!g_valve_open && min_dryness > g_thresh_high) valve_set(true);
    }
}

// log task
static void log_task(void *arg) {
    const TickType_t every = pdMS_TO_TICKS(5000);
    while (1) {
        vTaskDelay(every);
        xSemaphoreTake(g_sensors_mutex, portMAX_DELAY);
        for (int i = 0; i < ctx.num_sensors; ++i) {
            ESP_LOGI(TAG, "S%d raw=%4d dryness=%.3f",
                     ctx.sensors[i].id,
                     ctx.sensors[i].raw_level,
                     ctx.sensors[i].dryness_level);
        }
        xSemaphoreGive(g_sensors_mutex);
    }
}

void app_main(void){
    // NVS init
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // sensor inits
    moisture_sensor_init(&sensors[0], MOISTURE_IN_D32, MOISTURE_OUT_D4, SENSOR_ID_1);
    moisture_sensor_init(&sensors[1], MOISTURE_IN_D32, MOISTURE_OUT_D4, SENSOR_ID_2);

    // rtos primitives
    g_readings_q = xQueueCreate(32, sizeof(reading_t));
    g_sensors_mutex = xSemaphoreCreateMutex();
    configASSERT(g_readings_q && g_sensors_mutex);
    
    // network init
    wifi_init_ap(); // default ip: 192.168.4.1
    ESP_ERROR_CHECK(start_http_server(&ctx));

    // rtos tasks
    xTaskCreate(ingest_task, "ingest", STK_DEFAULT_WORDS, NULL, PRIO_HIGH, NULL);
    xTaskCreate(control_task, "control", STK_DEFAULT_WORDS, NULL, PRIO_HIGH, NULL);
    xTaskCreate(log_task, "log", STK_DEFAULT_WORDS, NULL, PRIO_LOW,  NULL);
}