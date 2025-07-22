#pragma once

#include "sensor.h"

typedef struct {
    MoistureSensor *sensors;
    int num_sensors;
} SensorContext;