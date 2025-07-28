#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "sensor.h"
#include "http_server.h"
#include "wifi_manager.h"
#include "SensorContext.h"

// GPIO pins for output: 2, 4, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
#define RELAY_OUT_D2 2
#define MOISTURE_OUT_D4 4

// GPIO pins for ADC input: 32 ADC1_CHANNEL_4, 33 ADC1_CHANNEL_5, 34 ADC1_CHANNEL_6, 35 ADC1_CHANNEL_7
#define MOISTURE_IN_D32 ADC1_CHANNEL_4

#define NUM_SENSORS 2 // number of sensors used

#define SENSOR_ID_1 1
#define SENSOR_ID_2 2

void app_main(void){

    static MoistureSensor sensors[NUM_SENSORS];
    moisture_sensor_init(&sensors[0], MOISTURE_IN_D32, MOISTURE_OUT_D4, SENSOR_ID_1);
    moisture_sensor_init(&sensors[1], MOISTURE_IN_D32, MOISTURE_OUT_D4, SENSOR_ID_2);

    SensorContext ctx = {
        .sensors = sensors,
        .num_sensors = NUM_SENSORS
    };

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    wifi_init_ap();
    start_http_server(&ctx);

    while(1){
        /*
        moisture_sensor_update(&sensor1);
        raw_level_1 = moisture_sensor_get_raw(&sensor1);
        moisture_level_1 = moisture_sensor_get_percent(&sensor1);
        printf("Moisture Level 1: %f, Raw Level 1: %d\n", moisture_level_1, raw_level_1);
        */
        printf("Dryness Level 1: %f, Raw Level 1: %d\n", sensors[0].dryness_level, sensors[0].raw_level);
        vTaskDelay(pdMS_TO_TICKS(1000)); // delay for 1 second
    }
}