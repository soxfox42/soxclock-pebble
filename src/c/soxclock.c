#include <pebble.h>

static Window *s_main_window;

static BitmapLayer *s_background_layer;
static TextLayer *s_date_layer;
static Layer *s_battery_layer;
static Layer *s_steps_layer;
static Layer *s_hands_layer;

static GBitmap *s_background_bitmap;
static GBitmap *s_battery_bitmap;
static GBitmap *s_charging_bitmap;
static GBitmap *s_steps_bitmap;

static int s_time_hours;
static int s_time_minutes;
static char s_date[32];
static int s_battery_level;
static bool s_battery_charging;

static void update_time() {
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
    layer_mark_dirty(s_steps_layer);
}

static void battery_handler(BatteryChargeState state) {
    s_battery_level = state.charge_percent;
    s_battery_charging = state.is_charging;
    layer_mark_dirty(s_battery_layer);
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

static void draw_meter(Layer *layer, GContext *ctx, int percentage) {
    GRect bounds = layer_get_bounds(layer);

    #ifdef PBL_BW
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_line(ctx, GPoint(0, bounds.size.h - 1), GPoint(bounds.size.w - 1, bounds.size.h - 1));
    #else
        graphics_context_set_stroke_color(ctx, GColorDarkGray);
        graphics_context_set_stroke_width(ctx, 3);
        graphics_draw_line(ctx, GPoint(0, bounds.size.h - 2), GPoint(bounds.size.w - 1, bounds.size.h - 2));
    #endif

    int width = (bounds.size.w - 1) * percentage / 100;
    if (width > 0) {
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_stroke_width(ctx, 3);
        graphics_draw_line(ctx, GPoint(0, bounds.size.h - 2), GPoint(width, bounds.size.h - 2));
    }

}

static void battery_layer_update(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

    if (s_battery_charging) {
        graphics_draw_bitmap_in_rect(ctx, s_charging_bitmap, GRect(2, 2, 20, 20));
    } else {
        graphics_draw_bitmap_in_rect(ctx, s_battery_bitmap, GRect(2, 2, 20, 20));
        graphics_context_set_fill_color(ctx, GColorWhite);

        int width = 13 * s_battery_level / 100;
        graphics_fill_rect(ctx, GRect(5, 10, width, 4), 0, GCornerNone);
    }

    draw_meter(layer, ctx, s_battery_level);

    char label[5];
    snprintf(label, sizeof(label), "%d%%", s_battery_level);
    graphics_draw_text(
        ctx,
        label,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(0, 0, bounds.size.w, 18),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentRight,
        NULL
    );
}

static void steps_layer_update(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int steps = health_service_sum_today(HealthMetricStepCount);

    graphics_draw_bitmap_in_rect(ctx, s_steps_bitmap, GRect(2, 2, 20, 20));

    int percentage = steps <= 8000 ? steps * 100 / 8000 : 100;
    draw_meter(layer, ctx, percentage);

    char label[7];
    snprintf(label, sizeof(label), "%d", steps);
    graphics_draw_text(
        ctx,
        label,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(0, 0, bounds.size.w, 18),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentRight,
        NULL
    );
}

static void main_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
    s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATTERY);
    s_charging_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CHARGING);
    s_steps_bitmap = gbitmap_create_with_resource(RESOURCE_ID_STEPS);

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

    s_battery_layer = layer_create(
        GRect(meter_inset_horizontal, meter_inset_vertical, bounds.size.w - meter_inset_horizontal * 2, 24)
    );
    layer_set_update_proc(s_battery_layer, battery_layer_update);
    layer_add_child(window_layer, s_battery_layer);
    battery_handler(battery_state_service_peek());

    s_steps_layer = layer_create(
        GRect(meter_inset_horizontal, bounds.size.h - 24 - meter_inset_vertical, bounds.size.w - meter_inset_horizontal * 2, 24)
    );
    layer_set_update_proc(s_steps_layer, steps_layer_update);
    layer_add_child(window_layer, s_steps_layer);

    int size = bounds.size.w;
    int vertical_padding = (bounds.size.h - size) / 2;
    s_hands_layer = layer_create(GRect(0, vertical_padding, size, size));
    layer_set_update_proc(s_hands_layer, hands_layer_update);
    layer_add_child(window_layer, s_hands_layer);
}

static void main_window_unload(Window *window) {
    gbitmap_destroy(s_background_bitmap);
    gbitmap_destroy(s_battery_bitmap);
    gbitmap_destroy(s_charging_bitmap);
    gbitmap_destroy(s_steps_bitmap);

    bitmap_layer_destroy(s_background_layer);
    text_layer_destroy(s_date_layer);
    layer_destroy(s_battery_layer);
    layer_destroy(s_hands_layer);
}

static void init() {
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_handler);

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

static void deinit() {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    window_destroy(s_main_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
