#include "steps_layer.h"

#include "meter.h"

static Layer *s_steps_layer;

static GBitmap *s_steps_bitmap;

static void steps_layer_update(Layer *layer, GContext *ctx) {
    int steps = health_service_sum_today(HealthMetricStepCount);
    int percent = steps <= 8000 ? steps * 100 / 8000 : 100;
    char text[7];
    snprintf(text, sizeof(text), "%d", steps);

    draw_meter2(layer, ctx, s_steps_bitmap, percent, text);
}

void steps_layer_init(Layer *window_layer, GRect bounds) {
    s_steps_bitmap = gbitmap_create_with_resource(RESOURCE_ID_STEPS);

    s_steps_layer = layer_create(bounds);
    layer_set_update_proc(s_steps_layer, steps_layer_update);
    layer_add_child(window_layer, s_steps_layer);
}

void steps_layer_set_bounds(GRect bounds);

void steps_layer_deinit(void) {
    gbitmap_destroy(s_steps_bitmap);
    layer_destroy(s_steps_layer);
}

void steps_layer_mark_dirty(void) {
    layer_mark_dirty(s_steps_layer);
}

