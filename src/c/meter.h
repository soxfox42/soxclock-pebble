#pragma once

#include <pebble.h>

void draw_meter(Layer *layer, GContext *ctx, int percentage);
void draw_meter2(Layer *layer, GContext *ctx, const GBitmap *icon, int percent, const char *text);
