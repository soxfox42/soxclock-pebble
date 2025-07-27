#include "meter.h"

void draw_meter(Layer *layer, GContext *ctx, int percentage) {
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

static void draw_meter_line(Layer *layer, GContext *ctx, int percent) {
    draw_meter(layer, ctx, percent);
}

void draw_meter2(Layer *layer, GContext *ctx, const GBitmap *icon, int percent, const char *text) {
    GRect bounds = layer_get_bounds(layer);

    graphics_draw_bitmap_in_rect(ctx, icon, GRect(2, 2, bounds.size.h - 4, bounds.size.h - 4));

    draw_meter_line(layer, ctx, percent);

    graphics_draw_text(
        ctx,
        text,
        fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
        GRect(0, 0, bounds.size.w, 18),
        GTextOverflowModeTrailingEllipsis,
        GTextAlignmentRight,
        NULL
    );
}
