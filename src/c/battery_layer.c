#include "battery_layer.h"

#include "meter.h"

static Layer *s_battery_layer;

static GBitmap *s_battery_bitmap;
static GBitmap *s_charging_bitmap;

static int s_battery_level;
static bool s_battery_charging;

static void battery_handler(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
    s_battery_charging = state.is_charging;
    layer_mark_dirty(s_battery_layer);
}

static void battery_layer_update(Layer *layer, GContext *ctx) {
    char text[5];
    snprintf(text, sizeof(text), "%d%%", s_battery_level);

    draw_meter2(
        layer, ctx,
        s_battery_charging ? s_charging_bitmap : s_battery_bitmap,
        s_battery_level,
        text
    );

    if (!s_battery_charging) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        int width = 13 * s_battery_level / 100;
        graphics_fill_rect(ctx, GRect(5, 10, width, 4), 0, GCornerNone);
    }
}

void battery_layer_init(Layer *window_layer, GRect bounds) {
    s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATTERY);
    s_charging_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CHARGING);

    s_battery_layer = layer_create(bounds);
    layer_set_update_proc(s_battery_layer, battery_layer_update);
    layer_add_child(window_layer, s_battery_layer);

    battery_state_service_subscribe(battery_handler);
    battery_handler(battery_state_service_peek());
}

void battery_layer_set_bounds(GRect bounds);

void battery_layer_deinit(void) {
    gbitmap_destroy(s_battery_bitmap);
    gbitmap_destroy(s_charging_bitmap);
    layer_destroy(s_battery_layer);
}

