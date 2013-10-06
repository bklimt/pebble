
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xA9, 0x62, 0xFF, 0x4B, 0xF8, 0x29, 0x46, 0xE1, 0x9D, 0x70, 0x0A, 0x02, 0x54, 0x00, 0x1C, 0x5A }

#define WIDTH 144
#define HEIGHT 168

#define CENTER_X 72
#define CENTER_Y 84

static const GPoint CENTER = {
  .x = CENTER_X,
  .y = CENTER_Y
};

PBL_APP_INFO(MY_UUID,
             "Not Worst", "Not the Worst Apps",
             1, 0,  /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;
Layer hands_layer;

typedef struct {
  int x;
  int y;
  int slope_times_1000;
  bool slope_is_undefined;
} Hand;

void init_hand(Hand *hand, int value, int max_value) {
  value = value % max_value;

  hand->x = sin_lookup((TRIG_MAX_ANGLE * value) / max_value) * 1000 / TRIG_MAX_RATIO;
  hand->y = -cos_lookup((TRIG_MAX_ANGLE * value) / max_value) * 1000 / TRIG_MAX_RATIO;
  hand->slope_times_1000 = 0;
  hand->slope_is_undefined = (hand->x == 0);
  if (!hand->slope_is_undefined) {
    hand->slope_times_1000 = (hand->y * 1000) / hand->x;
  }
}

GColor get_hand_color(Hand *hand, int dx, int dy, bool reversed) {
  GColor color;
  if (hand->slope_is_undefined) {
    if (hand->y < 0) {
      // This is 0 seconds. Everything is white.
      color = GColorWhite;
    } else {
      // This is 30 seconds. Everything to the right is black.
      if (dx > 0) {
        color = GColorBlack;
      } else {
        color = GColorWhite;
      }
    }
  } else {
    bool below_line = (dy * 1000 > dx * hand->slope_times_1000);

    bool point_in_q1 = (dx >= 0 && dy <= 0);
    bool point_in_q2 = (dx >= 0 && dy > 0);
    bool point_in_q3 = (dx < 0 && dy > 0);
    bool point_in_q4 = (dx < 0 && dy <= 0);

    if (hand->x > 0 && hand->y <= 0) {
      // first quarter
      if (point_in_q1 && !below_line) {
        color = GColorBlack;
      } else {
        color = GColorWhite;
      }
    } else if (hand->x > 0) {
      // second quarter
      if (point_in_q1 || (point_in_q2 && !below_line)) {
        color = GColorBlack;
      } else {
        color = GColorWhite;
      }
    } else if (hand->y > 0) {
      // third quarter
      if (point_in_q1 || point_in_q2 || (point_in_q3 && below_line)) {
        color = GColorBlack;
      } else {
        color = GColorWhite;
      }
    } else {
      // fourth quarter
      if (point_in_q1 || point_in_q2 || point_in_q3 || (point_in_q4 && below_line)) {
        color = GColorBlack;
      } else {
        color = GColorWhite;
      }
    }
  }
  if (reversed) {
    color = (color == GColorBlack) ? GColorWhite : GColorBlack;
  }
  return color;
}

void hands_layer_update_callback(Layer *self, GContext *ctx) {
  PblTm time;
  get_time(&time);

  Hand hour, minute, second;
  init_hand(&hour, time.tm_hour, 12);
  init_hand(&minute, time.tm_min, 60);
  init_hand(&second, time.tm_sec, 60);

  int cx = CENTER_X;
  int cy = CENTER_Y;

  GPoint point;
  for (point.x = 0; point.x < WIDTH; ++point.x) {
    for (point.y = 0; point.y < HEIGHT; ++point.y) {
      int dx = point.x - cx;
      int dy = point.y - cy;
      int distance_from_center_squared = dx * dx + dy * dy;
      if (distance_from_center_squared > 4900) {
        distance_from_center_squared = 4900;
      }

      GColor color = GColorWhite;
      if (distance_from_center_squared < 23 * 23) {
        color = get_hand_color(&hour, dx, dy, time.tm_hour >= 12);
      } else if (distance_from_center_squared < 46 * 46) {
        color = get_hand_color(&minute, dx, dy, time.tm_hour % 2);
      } else if (distance_from_center_squared < 70 * 70) {
        color = get_hand_color(&second, dx, dy, time.tm_min % 2);
      }
      
      // int r = rand() % 60;
      // GColor color = (r < time.tm_sec) ? GColorBlack : GColorWhite;
      graphics_context_set_stroke_color(ctx, color);
      graphics_draw_pixel(ctx, point);
    }
  }

  /*
  char buffer[100];
  snprintf(buffer, 100, "%d:%d", time.tm_hour, time.tm_min);

  // graphics_context_set_fill_color(ctx, GColorWhite);
  // graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_text_color(ctx, time.tm_sec % 2 ? GColorWhite : GColorBlack);

  GRect bounds = layer_get_bounds(window_get_root_layer(&window));

  graphics_text_draw(
      ctx,
      buffer,
      // fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49),
      fonts_get_system_font(FONT_KEY_GOTHIC_28),
      bounds,
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter,
      NULL);
  */
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  layer_mark_dirty(&hands_layer);
}

void handle_init(AppContextRef ctx) {
  window_init(&window, "NTW Window Main");
  window_stack_push(&window, true  /* Animated */);

  layer_init(&hands_layer, window.layer.frame);
  hands_layer.update_proc = &hands_layer_update_callback;
  layer_add_child(&window.layer, &hands_layer);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };

  app_event_loop(params, &handlers);
}
