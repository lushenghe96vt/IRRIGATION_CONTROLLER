#pragma once 
#include "driver/adc.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct{
    int id;                  // Unique ID for each sensor for JSON parsing, starting from 1
    adc1_channel_t channel;  // ADC channel for moisture sensor
    int gpio_power;          // GPIO pin to power the moisture sensor
    int raw_level;           // Raw moisture level reading
    float moisture_level;    // Processed moisture level (0.0 to 1.0)

} MoistureSensor;

// initialization
void moisture_sensor_init(MoistureSensor *sensor, adc1_channel_t channel, int gpio_power, int id);

// power control functions
void moisture_sensor_power_on(MoistureSensor *sensor);
void moisture_sensor_power_off(MoistureSensor *sensor);

// read/polling functions
int moisture_sensor_get_raw(MoistureSensor *sensor);
float moisture_sensor_get_percent(MoistureSensor *sensor);
void moisture_sensor_update(MoistureSensor *sensor);

// For wireless communication
const char* moisture_sensor_as_json(MoistureSensor *sensor);

