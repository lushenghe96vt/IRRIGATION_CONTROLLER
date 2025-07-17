#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "sensor.h"
#include "http_server.h"
#include "wifi_manager.h"

// GPIO pins for output: 2, 4, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
#define RELAY_OUT_D2 2
#define MOISTURE_OUT_D4 4

// GPIO pins for ADC input: 32 ADC1_CHANNEL_4, 33 ADC1_CHANNEL_5, 34 ADC1_CHANNEL_6, 35 ADC1_CHANNEL_7
#define MOISTURE_IN_D32 ADC1_CHANNEL_4

#define WIFI_SSID     "BELL878"
#define WIFI_PASSWORD "CD4643CD6675"

void app_main(void){
    /*
    esp_rom_gpio_pad_select_gpio(MOISTURE_OUT_D4);
    gpio_set_direction(MOISTURE_OUT_D4, GPIO_MODE_OUTPUT);
    gpio_set_level(MOISTURE_OUT_D4, 1); // power moisture senseor

    float moisture_level_1 = 0; // raw moisture levels
    adc1_config_width(ADC_WIDTH_BIT_12);  // 12-bit: values from 0–4095
    adc1_config_channel_atten(MOISTURE_IN_D32, ADC_ATTEN_DB_11);  // 0–3.3V range
    */
    
    /*
    MoistureSensor sensor1;
    moisture_sensor_init(&sensor1, MOISTURE_IN_D32, MOISTURE_OUT_D4, 1);
    float moisture_level_1 = 0;
    int raw_level_1 = 0;
    */

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard; // Use wildcard matching for URI handlers

    wifi_init("BELL878", "CD4643CD6675"); // replace with wifi credentials
    start_http_server();


    while(1){
        /*
        moisture_sensor_update(&sensor1);
        raw_level_1 = moisture_sensor_get_raw(&sensor1);
        moisture_level_1 = moisture_sensor_get_percent(&sensor1);
        printf("Moisture Level 1: %f, Raw Level 1: %d\n", moisture_level_1, raw_level_1);
        */

        vTaskDelay(pdMS_TO_TICKS(1000)); // delay for 1 second
    }
}