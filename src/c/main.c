#include <pebble.h>

#include "battery_layer.h"
#include "steps_layer.h"

static Window *s_main_window;

static BitmapLayer *s_background_layer;
static TextLayer *s_date_layer;
static Layer *s_hands_layer;

static GBitmap *s_background_bitmap;

static int s_time_hours;
static int s_time_minutes;
static char s_date[32];

static void update_time(void) {
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    s_time_hours = tick_time->tm_hour;
    s_time_minutes = tick_time->tm_min;

    layer_mark_dirty(s_hands_layer);

    strftime(s_date, sizeof(s_date), "%b\n%e", tick_time);
    text_layer_set_text(s_date_layer, s_date);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
    steps_layer_mark_dirty();
}

static void draw_hand(GContext *ctx, GRect bounds, int length, int width, int angle, GColor color) {
    GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
    GPoint hand_end;
    hand_end.x = center.x + (sin_lookup(angle) * length / TRIG_MAX_RATIO);
    hand_end.y = center.y + (-cos_lookup(angle) * length / TRIG_MAX_RATIO);

    graphics_context_set_stroke_color(ctx, color);
    graphics_context_set_stroke_width(ctx, width);
    graphics_draw_line(ctx, center, hand_end);
}

static void hands_layer_update(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    int hour_hand_length = bounds.size.w / 4;
    int minute_hand_length = bounds.size.w * 2 / 5;

    int hand_width = bounds.size.w / 20;

    #ifdef PBL_BW
        draw_hand(ctx, bounds, minute_hand_length, hand_width + 4, TRIG_MAX_ANGLE * s_time_minutes / 60, GColorBlack);
        draw_hand(ctx, bounds, minute_hand_length, hand_width, TRIG_MAX_ANGLE * s_time_minutes / 60, GColorWhite);
        draw_hand(ctx, bounds, hour_hand_length, hand_width + 4, TRIG_MAX_ANGLE * (s_time_hours * 60 + s_time_minutes) / 720, GColorBlack);
        draw_hand(ctx, bounds, hour_hand_length, hand_width, TRIG_MAX_ANGLE * (s_time_hours * 60 + s_time_minutes) / 720, GColorWhite);
    #else
        draw_hand(ctx, bounds, minute_hand_length, hand_width, TRIG_MAX_ANGLE * s_time_minutes / 60, GColorFashionMagenta);
        draw_hand(ctx, bounds, hour_hand_length, hand_width, TRIG_MAX_ANGLE * (s_time_hours * 60 + s_time_minutes) / 720, GColorChromeYellow);
    #endif
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);

    s_background_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

    #ifdef PBL_RECT
        s_date_layer = text_layer_create(GRect(bounds.size.w - 38, -2, 36, 36));
        text_layer_set_text_alignment(s_date_layer, GTextAlignmentRight);
    #else
        s_date_layer = text_layer_create(GRect(bounds.size.w / 2 - 18, 20, 36, 36));
        text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    #endif
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    #if PBL_DISPLAY_WIDTH > 180
        text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    #else
        text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    #endif
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    const int meter_inset_vertical = PBL_DISPLAY_HEIGHT / 6;
    const int meter_inset_horizontal = PBL_DISPLAY_WIDTH * 2 / 9;

    battery_layer_init(
        window_layer,
        GRect(meter_inset_horizontal, meter_inset_vertical, bounds.size.w - meter_inset_horizontal * 2, 24)
    );
    steps_layer_init(
        window_layer,
        GRect(meter_inset_horizontal, bounds.size.h - 24 - meter_inset_vertical, bounds.size.w - meter_inset_horizontal * 2, 24)
    );

    int size = bounds.size.w;
    int vertical_padding = (bounds.size.h - size) / 2;
    s_hands_layer = layer_create(GRect(0, vertical_padding, size, size));
    layer_set_update_proc(s_hands_layer, hands_layer_update);
    layer_add_child(window_layer, s_hands_layer);
}

static void main_window_unload(Window *window) {
    gbitmap_destroy(s_background_bitmap);

    bitmap_layer_destroy(s_background_layer);
    text_layer_destroy(s_date_layer);
    battery_layer_deinit();
    steps_layer_deinit();
    layer_destroy(s_hands_layer);
}

static void init(void) {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    s_main_window = window_create();

    window_set_window_handlers(
        s_main_window,
        (WindowHandlers){
            .load = main_window_load,
            .unload = main_window_unload,
        }
    );

    window_stack_push(s_main_window, true);
    update_time();
}

static void deinit(void) {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
