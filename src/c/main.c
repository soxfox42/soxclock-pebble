#include <pebble.h>

#include "battery_layer.h"
#include "steps_layer.h"

static Window *s_main_window;

static Layer *s_background_layer;
static TextLayer *s_date_layer;
static Layer *s_hands_layer;

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

static void background_layer_update(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_unobstructed_bounds(layer);
    GRect full_bounds = layer_get_bounds(layer);

    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, full_bounds, 0, GCornerNone);

    GPoint center = grect_center_point(&bounds);

    const int tick_inset = 15;
    const int tick_size = 10;

    graphics_context_set_stroke_width(ctx, PBL_DISPLAY_WIDTH >= 200 ? 5 : 3);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    for (int tick = 0; tick < 12; tick++) {
        int tick_angle = (tick - 1) * TRIG_MAX_ANGLE / 12;
        GPoint start;
        GPoint end;
        #ifdef PBL_RECT
            int sign = 1 - 2 * (tick >= 6);
            if (tick % 6 < 3) {
                start.x = center.x + sign * (center.x - tick_inset);
                start.y = center.y + sign * (center.x - tick_inset) * sin_lookup(tick_angle) / cos_lookup(tick_angle);
            } else {
                start.x = center.x + sign * (center.y - tick_inset) * cos_lookup(tick_angle) / sin_lookup(tick_angle);
                start.y = center.y + sign * (center.y - tick_inset);
            }
        #else
            start.x = center.x + (center.x - tick_inset) * cos_lookup(tick_angle) / TRIG_MAX_RATIO;
            start.y = center.y + (center.y - tick_inset) * sin_lookup(tick_angle) / TRIG_MAX_RATIO;
        #endif
        end.x = start.x + tick_size * cos_lookup(tick_angle) / TRIG_MAX_RATIO;
        end.y = start.y + tick_size * sin_lookup(tick_angle) / TRIG_MAX_RATIO;
        graphics_draw_line(ctx, start, end);
    }
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
    GRect bounds = layer_get_unobstructed_bounds(layer);

    int hour_hand_length = bounds.size.h / 5;
    int minute_hand_length = bounds.size.h / 3;

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

static GRect get_date_bounds(GRect bounds) {
    #ifdef PBL_RECT
        return GRect(bounds.size.w - 38, bounds.size.h - PBL_DISPLAY_HEIGHT - 2, 36, 36);
    #else
        return GRect(bounds.size.w - 50, bounds.size.h / 2 - 18, 36, 36);
    #endif
}

static GRect get_battery_bounds(GRect bounds) {
    int inset_h = bounds.size.w * 2 / 9;
    int inset_v = bounds.size.h / 6;
    return GRect(inset_h, inset_v, bounds.size.w - inset_h * 2, 24);
}

static GRect get_steps_bounds(GRect bounds) {
    int inset_h = bounds.size.w * 2 / 9;
    int inset_v = bounds.size.h / 6;
    return GRect(inset_h, bounds.size.h - inset_v - 24, bounds.size.w - inset_h * 2, 24);
}

static void unobstructed_area_change(AnimationProgress progress, void *context) {
    GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_main_window));
    layer_set_frame(text_layer_get_layer(s_date_layer), get_date_bounds(bounds));
    battery_layer_set_bounds(get_battery_bounds(bounds));
    steps_layer_set_bounds(get_steps_bounds(bounds));
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect full_bounds = layer_get_bounds(window_layer);
    GRect bounds = layer_get_unobstructed_bounds(window_layer);

    s_background_layer = layer_create(full_bounds);
    layer_set_update_proc(s_background_layer, background_layer_update);
    layer_add_child(window_layer, s_background_layer);

    s_date_layer = text_layer_create(get_date_bounds(bounds));
    text_layer_set_text_alignment(s_date_layer, PBL_IF_RECT_ELSE(GTextAlignmentRight, GTextAlignmentCenter));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    #if PBL_DISPLAY_WIDTH > 180
        text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    #else
        text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    #endif
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    battery_layer_init(window_layer, get_battery_bounds(bounds));
    steps_layer_init(window_layer, get_steps_bounds(bounds));

    s_hands_layer = layer_create(full_bounds);
    layer_set_update_proc(s_hands_layer, hands_layer_update);
    layer_add_child(window_layer, s_hands_layer);

    UnobstructedAreaHandlers handlers = {
        .change = unobstructed_area_change,
    };
    unobstructed_area_service_subscribe(handlers, NULL);
}

static void main_window_unload(Window *window) {
    battery_layer_deinit();
    steps_layer_deinit();
    layer_destroy(s_background_layer);
    layer_destroy(s_hands_layer);
    text_layer_destroy(s_date_layer);
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
