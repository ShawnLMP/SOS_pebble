#include <pebble.h>

#define pause 100 // ms paused
#define short_interval 100 // ms short vib
#define long_interval 400 // ms long vib

#define one_sec 1000 // 1000 ms

#define SOS_msg_key 1
#define text_buff_size 40 // # of char

static Window *window;
static TextLayer *text_layer;


static bool count_down_beginned = false;


static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *SOS_message = dict_find(iter, MESSAGE_KEY_SOSMessage);
  if(SOS_message && count_down_beginned) {
    char *msg = SOS_message->value->cstring;
    text_layer_set_text(text_layer, msg);
    persist_write_string(SOS_msg_key, msg);
  }
}




/*
Due to unknow issue, values in segments passed to VibePattern has to
be literally constant! Even "const int" doesn't work

Thus there is short_vib() and long_vib()
*/
static void short_vib() {
  light_enable(true);
  static const uint32_t const segments[] = {short_interval};
  VibePattern pattern = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pattern);
  psleep(short_interval);
  light_enable(false);
  psleep(pause);
}

static void long_vib() {
  light_enable(true);
  static const uint32_t const segments[] = {long_interval};
  VibePattern pattern = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pattern);
  psleep(long_interval);
  light_enable(false);
  psleep(pause);
}

static void SOS() {
  short_vib();
  short_vib();
  short_vib();
  
  long_vib();
  long_vib();
  long_vib();
  
  short_vib();
  short_vib();
  short_vib();
  
  app_timer_register(one_sec * 2, SOS, NULL); //repeat, without blocking
}

char text_buff[text_buff_size];
int count_down = 4;

void print_count_down() {
  snprintf(text_buff, text_buff_size, "SOS in : %d", count_down);
  text_layer_set_text(text_layer, text_buff);
}

static void count_down_begin(void *data) {
  if (count_down == 0) {
    persist_read_string(SOS_msg_key, text_buff, sizeof(char) * text_buff_size);
    text_layer_set_text(text_layer, text_buff);
    
    
    window_set_background_color(window, GColorRed);
    SOS();
  }
  else {
    print_count_down();
    count_down -= 1;
    app_timer_register(one_sec, count_down_begin, NULL);
  }
}


static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (count_down_beginned) {
    return;
  }
  count_down_beginned = true;
  count_down_begin(NULL);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
  count_down += 2;
  print_count_down();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
  if (count_down > 0) {
    count_down -= 2;    
  }
  print_count_down();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 20));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  
  print_count_down();
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  
  
  
  
  // Listen for AppMessages
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
  
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();
  deinit();
}