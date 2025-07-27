#pragma once

#include <pebble.h>

void steps_layer_init(Layer *window_layer, GRect bounds);
void steps_layer_set_bounds(GRect bounds);
void steps_layer_deinit(void);
void steps_layer_mark_dirty(void);

