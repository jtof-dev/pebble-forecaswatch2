#include "battery_layer.h"
#include "c/appendix/persist.h"

#define BATTERY_NUB_W 2
#define BATTERY_NUB_H 6
#define BATTERY_STROKE 1
#define FILL_PADDING 1


static Layer *s_battery_layer=NULL;
#define OPT_BATTERY_TEXT
#ifdef OPT_BATTERY_TEXT
static TextLayer *s_battery_text_layer=NULL;
    #ifndef BAT_FMT_STR
        #define BAT_FMT_STR "%d%%"
    #endif /* BAT_FMT_STR */
    #define MAX_BAT_STR "??%"  // When Battery is 100, the percent symbol is deliberately not shown (buffer full/truncated)

#endif  // OPT_BATTERY_TEXT

static void battery_state_handler(BatteryChargeState charge) {
    battery_layer_refresh();
}

static GColor get_battery_color(int level) {
    if (level >= 50)
        return GColorGreen;
    else if (level >= 30)
        return GColorYellow;
    else
        return GColorRed;
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);
    int w = bounds.size.w;
    int h = bounds.size.h;
    int battery_w = w - BATTERY_NUB_W;
    BatteryChargeState battery_state = battery_state_service_peek();
    int battery_level = battery_state.charge_percent;

#ifdef OPT_BATTERY_TEXT

    static char battery_text[] = MAX_BAT_STR;

    if (battery_state.is_charging)
    {
        snprintf(battery_text, sizeof(battery_text), "Chg");
    }
    else
    {
        snprintf(battery_text, sizeof(battery_text), BAT_FMT_STR, battery_state.charge_percent);
    }
    text_layer_set_text_color(s_battery_text_layer, GColorWhite);  // TODO? PBL_IF_COLOR_ELSE(get_battery_color(battery_level), GColorWhite)
    text_layer_set_text(s_battery_text_layer, battery_text);
    #endif  // OPT_BATTERY_TEXT

    // Fill the battery level
    GRect color_bounds = GRect(
        BATTERY_STROKE + FILL_PADDING, BATTERY_STROKE + FILL_PADDING,
        battery_w - (BATTERY_STROKE + FILL_PADDING) * 2, h - (BATTERY_STROKE + FILL_PADDING) * 2);
    GRect color_area = GRect(
        color_bounds.origin.x, color_bounds.origin.y,
        color_bounds.size.w * (float) (battery_level + 10) / (100.0 + 10), color_bounds.size.h);
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(get_battery_color(battery_level), GColorWhite));
    graphics_fill_rect(ctx, color_area, 0, GCornerNone);

    // Draw the white battery outline
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, BATTERY_STROKE);
    graphics_draw_rect(ctx, GRect(0, 0, battery_w, h));

    // Draw the battery nub on the right
    graphics_draw_rect(ctx, GRect(battery_w - 1, h / 2 - BATTERY_NUB_H / 2, BATTERY_NUB_W + 1, BATTERY_NUB_H));
}

void battery_layer_create(Layer* parent_layer, GRect frame) {
    // battery text needs to be ABOVE calendar status
    //#define CALENDAR_STATUS_HEIGHT 13
    // Set up battery text text layer, mirror month (font) settings
    //s_battery_text_layer = text_layer_create(GRect(0, -MONTH_FONT_OFFSET, w, 25));  // TODO use frame and do math on it
    s_battery_text_layer = text_layer_create(GRect(0, 0, 25, 25));
    text_layer_set_background_color(s_battery_text_layer, GColorClear);
    //text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentCenter);
    text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentRight);
    text_layer_set_text_color(s_battery_text_layer, GColorWhite);
    text_layer_set_font(s_battery_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

    s_battery_layer = layer_create(frame);
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    battery_state_service_subscribe(battery_state_handler);
    layer_add_child(parent_layer, s_battery_layer);

    layer_add_child(parent_layer, s_battery_text_layer);
}

void battery_layer_refresh() {
    layer_mark_dirty(s_battery_layer);
}

void battery_layer_destroy() {
    battery_state_service_unsubscribe();
    if (s_battery_layer)
        layer_destroy(s_battery_layer);
    if (s_battery_text_layer)
        text_layer_destroy(s_battery_text_layer);
}
