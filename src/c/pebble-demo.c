#include <pebble.h>
#include <pebble-events/pebble-events.h>

static Window *s_main_window;
static TextLayer *s_hour_layer;
static TextLayer *s_minute_layer;
static TextLayer *s_date_layer;
static TextLayer *s_steps_layer;
static TextLayer *s_distance_layer;
static TextLayer *s_temp_layer;
static TextLayer *s_battery_layer;
static GFont s_hour_font;
static GFont s_minute_font;
static GFont s_small_font;
static uint32_t weatherKey = 1;
static uint32_t settingsKey = 2;
static char openWeatherMapKey[] = "066e2cfef62d346a166dc3902674a116";

typedef struct ClaySettings
{
  GColor BackgroundColor;
  GColor TimeColor;
  GColor DateColor;
  GColor WeatherColor;
  GColor HealthColor;
  bool IsHealthEnabled;
  bool IsDistanceMetricUnits;
  bool IsWeatherEnabled;
  bool IsTempMetricUnits;
  bool IsDateEnabled;
  bool IsBatteryEnabled;
} ClaySettings;

static ClaySettings settings;

static void load_default_settings()
{
  settings.BackgroundColor = GColorBlack;
  settings.TimeColor = GColorWhite;
  settings.DateColor = GColorWhite;
  settings.WeatherColor = GColorWhite;
  settings.HealthColor = GColorWhite;
  settings.IsHealthEnabled = true;
  settings.IsDistanceMetricUnits = true;
  settings.IsWeatherEnabled = true;
  settings.IsTempMetricUnits = true;
  settings.IsDateEnabled = true;
  settings.IsBatteryEnabled = true;
}

static GColor get_battery_color(int charge_percent)
{
  if (charge_percent > 30)
  {
    return GColorIslamicGreen;
  }
  else if (charge_percent > 10)
  {
    return GColorChromeYellow;
  }
  else
  {
    return GColorRed;
  }
}

static int c_to_f(int temp_c)
{
  return (int)((temp_c * 9) / 5) + 32;
}

static void set_temp_text(int temp_c)
{
  static char s_buffer[7];
  if (settings.IsTempMetricUnits)
  {
    snprintf(s_buffer, sizeof(s_buffer), "%d°C ", temp_c);
  }
  else
  {
    snprintf(s_buffer, sizeof(s_buffer), "%d°F ", c_to_f(temp_c));
  }
  text_layer_set_text(s_temp_layer, s_buffer);
}

static void set_distance_text()
{
  static char s_distance_buffer[8];

  if (settings.IsDistanceMetricUnits)
  {
    APP_LOG(APP_LOG_LEVEL_INFO, "Setting distance in metric");
    snprintf(s_distance_buffer, sizeof(s_distance_buffer), "%dKM", (int)health_service_sum_today(HealthMetricWalkedDistanceMeters) / 1000);
  }
  else
  {
    APP_LOG(APP_LOG_LEVEL_INFO, "Setting distance in miles");
    snprintf(s_distance_buffer, sizeof(s_distance_buffer), "%dMI", (int)health_service_sum_today(HealthMetricWalkedDistanceMeters) / 1609);
  }
  text_layer_set_text(s_distance_layer, s_distance_buffer);
}

static void update_display()
{
  window_set_background_color(s_main_window, settings.BackgroundColor);
  text_layer_set_text_color(s_hour_layer, settings.TimeColor);
  text_layer_set_text_color(s_minute_layer, settings.TimeColor);

  if (settings.IsHealthEnabled)
  {
    text_layer_set_text_color(s_steps_layer, settings.HealthColor);
    text_layer_set_text_color(s_distance_layer, settings.HealthColor);
    set_distance_text();
  }
  else
  {
    text_layer_set_text_color(s_steps_layer, GColorClear);
    text_layer_set_text_color(s_distance_layer, GColorClear);
  }

  if (settings.IsDateEnabled)
  {
    text_layer_set_text_color(s_date_layer, settings.DateColor);
  }
  else
  {
    text_layer_set_text_color(s_date_layer, GColorClear);
  }
  if (settings.IsWeatherEnabled)
  {
    text_layer_set_text_color(s_temp_layer, settings.WeatherColor);
    set_temp_text((int)persist_read_int(weatherKey));
  }
  else
  {
    text_layer_set_text_color(s_temp_layer, GColorClear);
  }
  if (settings.IsBatteryEnabled)
  {
    BatteryChargeState battery = battery_state_service_peek();
    text_layer_set_background_color(s_battery_layer, get_battery_color((int)battery.charge_percent));
  }
  else
  {
    text_layer_set_background_color(s_battery_layer, GColorClear);
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "Display updated per settings!");
}

static void load_settings()
{
  load_default_settings();
  persist_read_data(settingsKey, &settings, sizeof(settings));
  APP_LOG(APP_LOG_LEVEL_INFO, "Loaded settings!");
}

static bool fetch_weather()
{
  DictionaryIterator *out;
  AppMessageResult result = app_message_outbox_begin(&out);
  if (result != APP_MSG_OK)
  {
    return false;
  }

  dict_write_cstring(out, MESSAGE_KEY_Request, openWeatherMapKey);

  result = app_message_outbox_send();
  if (result != APP_MSG_OK)
  {
    return false;
  }

  return true;
}

static void update_time()
{
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_minute_buffer[4];
  static char s_hour_buffer[3];
  strftime(s_minute_buffer, sizeof(s_minute_buffer), ":%M", tick_time);
  strftime(s_hour_buffer, 3, "%l", tick_time);
  text_layer_set_text(s_minute_layer, s_minute_buffer);
  text_layer_set_text(s_hour_layer, s_hour_buffer);
  APP_LOG(APP_LOG_LEVEL_INFO, "Time set!");

  if (settings.IsDateEnabled)
  {
    static char s_date_buffer[15];
    strftime(s_date_buffer, sizeof(s_date_buffer), "%a, %b %e", tick_time);
    text_layer_set_text(s_date_layer, s_date_buffer);
    APP_LOG(APP_LOG_LEVEL_INFO, "Date set!");
  }
  if (settings.IsHealthEnabled)
  {
    static char s_steps_buffer[14];
    snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%d", (int)health_service_sum_today(HealthMetricStepCount));
    text_layer_set_text(s_steps_layer, s_steps_buffer);
    set_distance_text();

    APP_LOG(APP_LOG_LEVEL_INFO, "Heath set!");
  }
  if (settings.IsWeatherEnabled)
  {
    if (tick_time->tm_min == 0)
    {
      fetch_weather();
    }
  }
}

static void js_ready_handler(void *context)
{
  fetch_weather();
}

static void battery_callback(BatteryChargeState battery)
{
  text_layer_set_size(s_battery_layer, GSize(layer_get_bounds(window_get_root_layer(s_main_window)).size.w * ((int)battery.charge_percent / 100.0),
                                             3));
  text_layer_set_background_color(s_battery_layer, get_battery_color((int)battery.charge_percent));
  APP_LOG(APP_LOG_LEVEL_INFO, "Updated Battery: %d", (int)battery.charge_percent);
}

static void config_callback(DictionaryIterator *iter, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Config Update received!");

  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);

  if (bg_color_t)
  {
    APP_LOG(APP_LOG_LEVEL_INFO, "Updating configuration...");
    settings.BackgroundColor = GColorFromHEX(bg_color_t->value->int32);
  }

  Tuple *time_color_t = dict_find(iter, MESSAGE_KEY_TimeColor);
  if (time_color_t)
  {
    settings.TimeColor = GColorFromHEX(time_color_t->value->int32);
  }

  Tuple *date_color_t = dict_find(iter, MESSAGE_KEY_DateColor);
  if (date_color_t)
  {
    settings.DateColor = GColorFromHEX(date_color_t->value->int32);
  }

  Tuple *weather_color_t = dict_find(iter, MESSAGE_KEY_WeatherColor);
  if (weather_color_t)
  {
    settings.WeatherColor = GColorFromHEX(weather_color_t->value->int32);
  }

  Tuple *health_color_t = dict_find(iter, MESSAGE_KEY_HealthColor);
  if (health_color_t)
  {
    settings.HealthColor = GColorFromHEX(health_color_t->value->int32);
  }

  Tuple *is_health_enabled_t = dict_find(iter, MESSAGE_KEY_IsHealthEnabled);
  if (is_health_enabled_t)
  {
    settings.IsHealthEnabled = is_health_enabled_t->value->int32 == 1;
  }

  Tuple *is_distance_metric_t = dict_find(iter, MESSAGE_KEY_IsDistanceMetricUnits);
  if (is_distance_metric_t)
  {
    settings.IsDistanceMetricUnits = is_distance_metric_t->value->int32 == 1;
  }

  Tuple *is_weather_enabled_t = dict_find(iter, MESSAGE_KEY_IsWeatherEnabled);
  if (is_weather_enabled_t)
  {
    settings.IsWeatherEnabled = is_weather_enabled_t->value->int32 == 1;
  }

  Tuple *is_temp_metric_t = dict_find(iter, MESSAGE_KEY_IsTempMetricUnits);
  if (is_temp_metric_t)
  {
    settings.IsTempMetricUnits = is_temp_metric_t->value->int32 == 1;
  }

  Tuple *is_date_enabled_t = dict_find(iter, MESSAGE_KEY_IsDateEnabled);
  if (is_date_enabled_t)
  {
    settings.IsDateEnabled = is_date_enabled_t->value->int32 == 1;
  }

  Tuple *is_battery_enabled_t = dict_find(iter, MESSAGE_KEY_IsBatteryEnabled);
  if (is_battery_enabled_t)
  {
    // if battery was previously enabled but now enabled we need to subscribe
    if (!settings.IsBatteryEnabled && is_battery_enabled_t->value->int32 == 1)
    {
      APP_LOG(APP_LOG_LEVEL_INFO, "Configuration changed...  subscribing to battery service");
      battery_state_service_subscribe(battery_callback);
    }
    // else if the battery was previously enabled and is not disabled we need to unsubscribe
    else if (settings.IsBatteryEnabled && is_battery_enabled_t->value->int32 != 1)
    {
      APP_LOG(APP_LOG_LEVEL_INFO, "Configuration changed...  unsubscribing from battery service");
      battery_state_service_unsubscribe();
    }

    settings.IsBatteryEnabled = is_battery_enabled_t->value->int32 == 1;
  }

  persist_write_data(settingsKey, &settings, sizeof(settings));
  update_display();
}

static void weather_callback(DictionaryIterator *iter, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Weather Update received!");
  Tuple *temp_tuple = dict_find(iter, MESSAGE_KEY_TempK);
  static int temp_c = 0;
  temp_c = (int)(temp_tuple->value->int32 - 273);
  set_temp_text(temp_c);
  persist_write_int(weatherKey, temp_c);
  APP_LOG(APP_LOG_LEVEL_INFO, "Weather Updated: %d°C ", temp_c);
}

static void app_message_callback(DictionaryIterator *iter, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "AppMessage received!");

  Tuple *reply_t = dict_find(iter, MESSAGE_KEY_Reply);
  Tuple *bg_color_t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (reply_t)
  {
    weather_callback(iter, context);
  }
  else if (bg_color_t)
  {
    config_callback(iter, context);
  }
}

static void init_steps_layer(GRect window_bounds, Layer *window_layer)
{
  s_steps_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(155, 149), window_bounds.size.w, 20));
  text_layer_set_background_color(s_steps_layer, GColorClear);
  text_layer_set_font(s_steps_layer, s_small_font);
  if (settings.IsHealthEnabled)
  {
    text_layer_set_text_color(s_steps_layer, settings.HealthColor);
  }
  else
  {
    text_layer_set_text_color(s_steps_layer, GColorClear);
  }
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));

  APP_LOG(APP_LOG_LEVEL_INFO, "Steps initialized");
}

static void init_temp_layer(GRect window_bounds, Layer *window_layer)
{
  s_temp_layer = text_layer_create(
      GRect(0, 3, window_bounds.size.w, 20));
  text_layer_set_background_color(s_temp_layer, GColorClear);
  text_layer_set_font(s_temp_layer, s_small_font);

  set_temp_text((int)persist_read_int(weatherKey));

  if (settings.IsWeatherEnabled)
  {
    text_layer_set_text_color(s_temp_layer, settings.WeatherColor);
  }
  else
  {
    text_layer_set_text_color(s_temp_layer, GColorClear);
  }
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_temp_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "Weather initialized");
}

static void init_distance_layer(GRect window_bounds, Layer *window_layer)
{
  s_distance_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(155, 149), window_bounds.size.w, 20));
  text_layer_set_background_color(s_distance_layer, GColorClear);
  text_layer_set_font(s_distance_layer, s_small_font);

  if (settings.IsHealthEnabled)
  {
    text_layer_set_text_color(s_distance_layer, settings.HealthColor);
  }
  else
  {
    text_layer_set_text_color(s_distance_layer, GColorClear);
  }
  text_layer_set_text_alignment(s_distance_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_distance_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "Distance initialized");
}

static void init_date_layer(GRect window_bounds, Layer *window_layer)
{
  if (settings.IsDateEnabled)
  {
    s_date_layer = text_layer_create(
        GRect(0, 105, window_bounds.size.w, 22));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_font(s_date_layer, s_small_font);
    text_layer_set_text_color(s_date_layer, settings.DateColor);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
    APP_LOG(APP_LOG_LEVEL_INFO, "Date initialized");
  }
}

static void init_hour_layer(GRect window_bounds, Layer *window_layer)
{
  s_hour_layer = text_layer_create(
      GRect(0, 35, 82, 67));
  text_layer_set_background_color(s_hour_layer, GColorClear);
  text_layer_set_font(s_hour_layer, s_hour_font);
  text_layer_set_text_color(s_hour_layer, settings.TimeColor);
  text_layer_set_text_alignment(s_hour_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "Hour initialized");
}

static void init_minute_layer(GRect window_bounds, Layer *window_layer)
{
  s_minute_layer = text_layer_create(
      GRect(80, 47, window_bounds.size.w, 39));
  text_layer_set_background_color(s_minute_layer, GColorClear);
  text_layer_set_font(s_minute_layer, s_minute_font);
  text_layer_set_text_color(s_minute_layer, settings.TimeColor);
  text_layer_set_text_alignment(s_minute_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "Minute initialized");
}

static void init_battery_layer(GRect window_bounds, Layer *window_layer)
{
  if (settings.IsBatteryEnabled)
  {
    BatteryChargeState battery = battery_state_service_peek();
    s_battery_layer = text_layer_create(GRect(0, 0, (window_bounds.size.w * ((int)battery.charge_percent / 100.0)), 3));
    text_layer_set_background_color(s_battery_layer, get_battery_color((int)battery.charge_percent));
    layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
    APP_LOG(APP_LOG_LEVEL_INFO, "Battery initialized: %d", (int)battery.charge_percent);
  }
  else
  {
    s_battery_layer = text_layer_create(GRect(0, 0, window_bounds.size.w, 3));
    text_layer_set_background_color(s_battery_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
}

static void load_fonts()
{
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_16));
  s_hour_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_66));
  s_minute_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_DECIMAL_MEDIUM_32));
  APP_LOG(APP_LOG_LEVEL_INFO, "Fonts loaded");
}

static void main_window_load(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);
  load_fonts();
  init_hour_layer(window_bounds, window_layer);
  init_minute_layer(window_bounds, window_layer);
  init_date_layer(window_bounds, window_layer);
  init_steps_layer(window_bounds, window_layer);
  init_distance_layer(window_bounds, window_layer);
  init_temp_layer(window_bounds, window_layer);
  init_battery_layer(window_bounds, window_layer);
  update_time();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void main_window_unload(Window *window)
{
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_steps_layer);
  text_layer_destroy(s_distance_layer);
  text_layer_destroy(s_temp_layer);
  text_layer_destroy(s_battery_layer);
  fonts_unload_custom_font(s_hour_font);
  fonts_unload_custom_font(s_small_font);
}

static void init()
{
  load_settings();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  if (settings.IsBatteryEnabled)
  {
    APP_LOG(APP_LOG_LEVEL_INFO, "On init subscribing to battery service!");
    battery_state_service_subscribe(battery_callback);
  }

  s_main_window = window_create();
  window_set_window_handlers(
      s_main_window,
      (WindowHandlers){
          .load = main_window_load,
          .unload = main_window_unload});
  window_stack_push(s_main_window, true);
  events_app_message_request_inbox_size(2026);
  events_app_message_request_outbox_size(656);
  events_app_message_open();

  // Open AppMessage connection
  app_message_register_inbox_received(app_message_callback);

  app_timer_register(3000, js_ready_handler, NULL);
}

static void deinit()
{
  window_destroy(s_main_window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}
