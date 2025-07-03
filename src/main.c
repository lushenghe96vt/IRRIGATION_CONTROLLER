#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

// GPIO pins for output: 2, 4, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
#define RELAY_OUT_D2 2
#define MOISTURE_OUT_D4 4

// GPIO pins for ADC input: 32 ADC1_CHANNEL_4, 33 ADC1_CHANNEL_5, 34 ADC1_CHANNEL_6, 35 ADC1_CHANNEL_7
#define MOISTURE_IN_D32 ADC1_CHANNEL_4

void app_main(void){
    esp_rom_gpio_pad_select_gpio(RELAY_OUT_D2);
    gpio_set_direction(RELAY_OUT_D2, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(MOISTURE_OUT_D4);
    gpio_set_direction(MOISTURE_OUT_D4, GPIO_MODE_OUTPUT);
    gpio_set_level(MOISTURE_OUT_D4, 1); // power moisture senseor

    float moisture_level_1 = 0; // raw moisture levels
    adc1_config_width(ADC_WIDTH_BIT_12);  // 12-bit: values from 0–4095
    adc1_config_channel_atten(MOISTURE_IN_D32, ADC_ATTEN_DB_11);  // 0–3.3V range

    while(1){
        gpio_set_level(RELAY_OUT_D2, 1);
        printf("Relay ON\n");
        vTaskDelay(pdMS_TO_TICKS(250));

        gpio_set_level(RELAY_OUT_D2, 0);
        printf("Relay OFF\n");
        vTaskDelay(pdMS_TO_TICKS(250));

        moisture_level_1 = adc1_get_raw(MOISTURE_IN_D32);
        printf("Moisture Level: %f\n", moisture_level_1);
    }
}