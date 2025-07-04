#include "sensor.h"

// initialization
void moisture_sensor_init(MoistureSensor *sensor, adc1_channel_t channel, int gpio_power){
    sensor->channel = channel;
    sensor->gpio_power = gpio_power;
    sensor->raw_level = 0;
    sensor->moisture_level = 0.0f;

    gpio_set_direction(sensor->gpio_power, GPIO_MODE_OUTPUT);
    adc1_config_width(ADC_WIDTH_BIT_12);  // 12-bit: values from 0–4095
    adc1_config_channel_atten(sensor->channel, ADC_ATTEN_DB_11);  // 0–3.3V range
}

// power control functions
void moisture_sensor_power_on(MoistureSensor *sensor){
    gpio_set_level(sensor->gpio_power, 1);
}
void moisture_sensor_power_off(MoistureSensor *sensor){
    gpio_set_level(sensor->gpio_power, 0);
}

// read/polling functions
int moisture_sensor_get_raw(MoistureSensor *sensor){
    return sensor->raw_level;
}
float moisture_sensor_get_percent(MoistureSensor *sensor){
    return sensor->moisture_level;
}
void moisture_sensor_update(MoistureSensor *sensor){
    moisture_sensor_power_on(sensor);
    vTaskDelay(pdMS_TO_TICKS(100)); // allow sensor to stabilize
    
    // read moisuture level
    sensor->raw_level = adc1_get_raw(sensor->channel); 
    if(sensor->raw_level < 0) { // handles error case
        sensor->raw_level = 0;
        sensor->moisture_level = 0.0f;
    }
    else {
        sensor->moisture_level = (float)sensor->raw_level / 4095.0f; // converts to percentage assuming 12-bit ADC resolution
    }

    moisture_sensor_power_off(sensor);
}

// For wireless communication
const char* moisture_sensor_as_json(MoistureSensor *sensor){ // add battery level and timestamps later
    static char json_buffer[128];
    snprintf(json_buffer, sizeof(json_buffer), "{\"channel\":%d,\"gpio_power\":%d,\"gpio_relay\":%d,\"raw_level\":%d,\"moisture_level\":%.2f}",
             sensor->channel, sensor->gpio_power, sensor->gpio_relay, sensor->raw_level, sensor->moisture_level);
    return json_buffer;
}