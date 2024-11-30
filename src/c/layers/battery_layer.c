#include "battery_layer.h"
#include "c/appendix/persist.h"

#define BATTERY_NUB_W 2
#define BATTERY_NUB_H 6
#define BATTERY_STROKE 1
#define FILL_PADDING 1

// Define the keys for AppMessage and persistent storage
enum {
  MESSAGE_KEY_SHOW_BATTERY_PERCENT = 0,
  PERSIST_KEY_SHOW_BATTERY_PERCENT = 1,
};

static Layer *s_battery_layer = NULL;
static TextLayer *s_battery_text_layer = NULL;

#ifndef BAT_FMT_STR
#define BAT_FMT_STR "%d%%"
#endif /* BAT_FMT_STR */
#define MAX_BAT_STR \
  "??%" // When Battery is 100, the percent symbol is deliberately not shown

static void battery_state_handler(BatteryChargeState charge) {
  battery_layer_refresh();
}

// This function is not used, so it's commented out to avoid the warning
/* static GColor get_battery_color(int level) {
  if (level >= 50)
    return GColorGreen;
  else if (level >= 30)
    return GColorYellow;
  else
    return GColorRed;
} */

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int w = bounds.size.w;
  int h = bounds.size.h;
  int battery_w = w - BATTERY_NUB_W;
  BatteryChargeState battery_state = battery_state_service_peek();
  int battery_level = battery_state.charge_percent;

  static char battery_text[] = MAX_BAT_STR;

  if (battery_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "Chg");
  } else {
    snprintf(battery_text, sizeof(battery_text), BAT_FMT_STR,
             battery_state.charge_percent);
  }
  text_layer_set_text_color(s_battery_text_layer, GColorWhite);
  text_layer_set_text(s_battery_text_layer, battery_text);

  // Fill the battery level
  GRect color_bounds =
      GRect(BATTERY_STROKE + FILL_PADDING, BATTERY_STROKE + FILL_PADDING,
            battery_w - (BATTERY_STROKE + FILL_PADDING) * 2,
            h - (BATTERY_STROKE + FILL_PADDING) * 2);
  GRect color_area =
      GRect(color_bounds.origin.x, color_bounds.origin.y,
            color_bounds.size.w * (float)(battery_level + 10) / (100.0 + 10),
            color_bounds.size.h);
  graphics_context_set_fill_color(
      ctx, /* PBL_IF_COLOR_ELSE(get_battery_color(battery_level), */ GColorWhite);
  graphics_fill_rect(ctx, color_area, 0, GCornerNone);

  // Draw the white battery outline
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, BATTERY_STROKE);
  graphics_draw_rect(ctx, GRect(0, 0, battery_w, h));

  // Draw the battery nub on the right
  graphics_draw_rect(ctx, GRect(battery_w - 1, h / 2 - BATTERY_NUB_H / 2,
                                  BATTERY_NUB_W + 1, BATTERY_NUB_H));
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *show_battery_percent_tuple =
      dict_find(iterator, MESSAGE_KEY_SHOW_BATTERY_PERCENT);
  if (show_battery_percent_tuple) {
    bool showBatteryPercent = show_battery_percent_tuple->value->int32 == 1;

    // Store the setting value
    persist_write_bool(PERSIST_KEY_SHOW_BATTERY_PERCENT, showBatteryPercent);

    // Update the watchface to show/hide the battery percentage text
    layer_set_hidden(text_layer_get_layer(s_battery_text_layer),
                     !showBatteryPercent);
  }
}

void battery_layer_create(Layer *parent_layer, GRect frame) {
  s_battery_text_layer =
      text_layer_create(GRect(frame.origin.x - 30, frame.origin.y - 8, 30,
                              20)); // WORKS matches level of month and year
  text_layer_set_background_color(s_battery_text_layer, GColorClear);
  text_layer_set_text_alignment(s_battery_text_layer, GTextAlignmentRight);
  text_layer_set_text_color(s_battery_text_layer, GColorWhite);
  text_layer_set_font(s_battery_text_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18));

  s_battery_layer = layer_create(frame);
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  battery_state_service_subscribe(battery_state_handler);
  layer_add_child(parent_layer, s_battery_layer);

  layer_add_child(parent_layer, text_layer_get_layer(s_battery_text_layer));

  // Initialize AppMessage
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(app_message_inbox_size_maximum(),
                    app_message_outbox_size_maximum());
}

void battery_layer_refresh() { layer_mark_dirty(s_battery_layer); }

void battery_layer_destroy() {
  battery_state_service_unsubscribe();
  if (s_battery_layer)
    layer_destroy(s_battery_layer);
  if (s_battery_text_layer)
    text_layer_destroy(s_battery_text_layer);
}