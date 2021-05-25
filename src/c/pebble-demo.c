#include <pebble.h>
#include <pebble-owm-weather/owm-weather.h>
#include <pebble-events/pebble-events.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_steps_layer;
static TextLayer *s_distance_layer;
static TextLayer *s_temp_layer;
static GFont s_time_font;
static GFont s_small_font;

static void weather_callback(OWMWeatherInfo *info, OWMWeatherStatus status)
{
  switch (status)
  {
  case OWMWeatherStatusAvailable:
  {
    static char s_buffer[7];
    // snprintf(s_buffer, sizeof(s_buffer),
    //   "Temperature (K/C/F): %d/%d/%d\n\nDescription/short:\n%s/%s\n\nPressure: %d\n\nWind speed/dir: %d/%d",
    //   info->temp_k, info->temp_c, info->temp_f, info->description,
    //   info->description_short, info->pressure, info->wind_speed, info->wind_direction);

    snprintf(s_buffer, sizeof(s_buffer), "%d°F", info->temp_f);
    text_layer_set_text(s_temp_layer, s_buffer);
  }
  break;
  case OWMWeatherStatusNotYetFetched:
    text_layer_set_text(s_temp_layer, "NotYetFetched");
    break;
  case OWMWeatherStatusBluetoothDisconnected:
    text_layer_set_text(s_temp_layer, "BluetoothDisconnected");
    break;
  case OWMWeatherStatusPending:
    text_layer_set_text(s_temp_layer, "Pending");
    break;
  case OWMWeatherStatusFailed:
    text_layer_set_text(s_temp_layer, "Failed");
    break;
  case OWMWeatherStatusBadKey:
    text_layer_set_text(s_temp_layer, "BadKey");
    break;
  case OWMWeatherStatusLocationUnavailable:
    text_layer_set_text(s_temp_layer, "LocationUnavailable");
    break;
  }
}

static void js_ready_handler(void *context)
{
  owm_weather_fetch(weather_callback);
}

static void update_time()
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_time_buffer[6];
  static char s_date_buffer[15];
  static char s_steps_buffer[14];
  static char s_distance_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), "%l:%M", tick_time);
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a, %b %e", tick_time);
  snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d", (int)health_service_sum_today(HealthMetricStepCount));
  snprintf(s_distance_buffer, sizeof(s_distance_buffer), "%dKM", (int)health_service_sum_today(HealthMetricWalkedDistanceMeters) / 1000);

  text_layer_set_text(s_time_layer, s_time_buffer);
  text_layer_set_text(s_date_layer, s_date_buffer);
  text_layer_set_text(s_steps_layer, s_steps_buffer);
  text_layer_set_text(s_distance_layer, s_distance_buffer);
}

static void init_steps_layer(GRect window_bounds, Layer *window_layer)
{
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_16));
  s_steps_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(155, 149), window_bounds.size.w, 20));
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_text_color(s_steps_layer, GColorWhite);
  text_layer_set_font(s_steps_layer, s_small_font);
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));
}

static void init_temp_layer(GRect window_bounds, Layer *window_layer)
{
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_16));
  s_temp_layer = text_layer_create(
      GRect(0, 0, window_bounds.size.w, 20));
  text_layer_set_background_color(s_temp_layer, GColorClear);
  text_layer_set_text_color(s_temp_layer, GColorWhite);
  text_layer_set_font(s_temp_layer, s_small_font);
  text_layer_set_text(s_temp_layer, "Ready...");
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_temp_layer));
}

static void init_distance_layer(GRect window_bounds, Layer *window_layer)
{
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_16));
  s_distance_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(155, 149), window_bounds.size.w, 20));
  text_layer_set_background_color(s_distance_layer, GColorClear);
  text_layer_set_text_color(s_distance_layer, GColorWhite);
  text_layer_set_font(s_distance_layer, s_small_font);
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));
}

static void init_date_layer(GRect window_bounds, Layer *window_layer)
{
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_16));
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(105, 99), window_bounds.size.w, 22));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, s_small_font);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
}

static void init_time_layer(GRect window_bounds, Layer *window_layer)
{
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_42));
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52), window_bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
}

static void main_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);
  init_time_layer(window_bounds, window_layer);
  init_date_layer(window_bounds, window_layer);
  init_steps_layer(window_bounds, window_layer);
  init_distance_layer(window_bounds, window_layer);
  init_temp_layer(window_bounds, window_layer);
  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void main_window_unload(Window *window)
{
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_steps_layer);
  text_layer_destroy(s_distance_layer);
  text_layer_destroy(s_temp_layer);
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_small_font);
}

static void init()
{
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  s_main_window = window_create();
  window_set_window_handlers(
      s_main_window,
      (WindowHandlers){
          .load = main_window_load,
          .unload = main_window_unload});
  window_stack_push(s_main_window, true);

  owm_weather_init("066e2cfef62d346a166dc3902674a116");
  events_app_message_open();

  app_timer_register(10000, js_ready_handler, NULL);
}

static void deinit()
{
  owm_weather_deinit();
  window_destroy(s_main_window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}
