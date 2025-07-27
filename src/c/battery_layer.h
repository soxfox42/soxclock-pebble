#pragma once

#include <pebble.h>

void battery_layer_init(Layer *window_layer, GRect bounds);
void battery_layer_set_bounds(GRect bounds);
void battery_layer_deinit(void);

