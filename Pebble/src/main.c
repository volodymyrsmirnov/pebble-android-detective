#include <pebble.h>
  
#define NUMBER_OF_SECTIONS 2
#define FREE_WIFI_SECTION "Public networks"
#define PROTECTED_WIFI_SECTION "Protected networks"
#define MAX_NETWORKS_IN_MESSAGE 32
  
#define WFD_TYPE 0
#define WFD_ENCRYPTED 2
#define WFD_QUALITY 3
#define WFD_SSID 4
#define WFD_BSSID 5
#define WFD_VIBE 6
  
#define WFD_RESPONSE_RESET 0
#define WFD_RESPONSE_NETWORK 1

// Macros for extracting uint8 from the Tuple
#define UINT8_VALUE(tuple) tuple->value->data[0]

// Window and menu
Window *window;
static MenuLayer *menu_layer;

// Signal quality icons
static GBitmap *signal_icons[4];

// Network typedef
typedef struct {
  char ssid[33];
  char bssid[18];
  uint8_t signal;
} network;

// Static data storage structure
struct networks_struct {
  network public[MAX_NETWORKS_IN_MESSAGE];
  network private[MAX_NETWORKS_IN_MESSAGE];

  uint8_t public_network_count;
  uint8_t private_network_count;
} networks;

// Data receiver
void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *type = dict_find(received, WFD_TYPE);
  
  // If mesage type is RESET - clean the iterators
  if (UINT8_VALUE(type) == WFD_RESPONSE_RESET) {
    networks.public_network_count = 0;
    networks.private_network_count = 0;
  } 
  // Otherwise - fill networks
  else if (UINT8_VALUE(type) == WFD_RESPONSE_NETWORK) {
    
    // Get the response dict values
    Tuple *encrypted = dict_find(received, WFD_ENCRYPTED);
    Tuple *quality = dict_find(received, WFD_QUALITY);
    Tuple *ssid = dict_find(received, WFD_SSID);
    Tuple *bssid = dict_find(received, WFD_BSSID);
    Tuple *vibe = dict_find(received, WFD_VIBE);
    
    // Vibrate if new network found
    if (UINT8_VALUE(vibe) == 1) {
      vibes_double_pulse();
    }
    
    // Fill private network
    if (UINT8_VALUE(encrypted) == 1) {
      strcpy(networks.private[networks.private_network_count].ssid, ssid->value->cstring);
      strcpy(networks.private[networks.private_network_count].bssid, bssid->value->cstring);
      networks.private[networks.private_network_count].signal = UINT8_VALUE(quality);
      networks.private_network_count++;
    }
    // Fill public network
    else {
      strcpy(networks.public[networks.public_network_count].ssid, ssid->value->cstring);
      strcpy(networks.public[networks.public_network_count].bssid, bssid->value->cstring);
      networks.public[networks.public_network_count].signal = UINT8_VALUE(quality);
      networks.public_network_count++;
    }
    
    // Redraw menu
    menu_layer_reload_data(menu_layer);
  }
}
  
// Get number of sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUMBER_OF_SECTIONS;
}

// Return the number of rows in section
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    // Public networks counter
    case 0:
      if (networks.public_network_count == 0) {
        return 1;
      }

      return networks.public_network_count;

      break;
    
    // Private networks counter
    case 1:
      if (networks.private_network_count == 0) {
        return 1;
      }
    
      return networks.private_network_count;

      break;
    
    default:
      return 0;
  }
}

// Get the height of a header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Draw section header
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0: 
      return menu_cell_basic_header_draw(ctx, cell_layer, FREE_WIFI_SECTION);
      break;
    
    case 1:
      return menu_cell_basic_header_draw(ctx, cell_layer, PROTECTED_WIFI_SECTION);;
      break; 
  }
}

// Event on row display
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char draw_empty = 0;
  network current;
  
  // Get the section and detect if it has items
  switch (cell_index->section) {
    case 0:
      if (networks.public_network_count == 0) {
        draw_empty = 1;
      }
      else {
        current = networks.public[cell_index->row];
      }
    
      break;
  
    case 1:
      if (networks.private_network_count == 0) {
        draw_empty = 1;
      }
      else {
        current = networks.private[cell_index->row];
      }
    
      break;
  }
  
  // Draw empty cell or network row 
  if (draw_empty == 1) {
    menu_cell_basic_draw(ctx, cell_layer, "Vacuum", "No networks detected", NULL);
  }
  else {
    // Select proper icon based on the signal strength
    uint8_t signal_identifier = 0;
    
    if (current.signal >= 80) {
      signal_identifier = 3;
    }
    else if (current.signal <= 79 && current.signal >= 50) {
      signal_identifier = 2;
    }
    else if (current.signal <= 49 && current.signal >= 20) {
      signal_identifier = 1;
    }
    
    menu_cell_basic_draw(ctx, cell_layer, current.ssid, current.bssid, signal_icons[signal_identifier]);
  }
}

// Even on window load
void window_load(Window *window) {
  // Get the window layer
  Layer *window_layer = window_get_root_layer(window);

  // Create menu layer
  menu_layer = menu_layer_create(layer_get_frame(window_layer));
  
  // Set appropriate callbacks
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
  });
  
  // Allow interaction with menu layer
  menu_layer_set_click_config_onto_window(menu_layer, window);
  
  // Draw menu layer inside of the window
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

int main(void) {
  // Hander for received data
  app_message_register_inbox_received(in_received_handler);
  
  // Set max buffer size
  app_message_open(128, 128);
  
  // Initialize signal strength icons
  signal_icons[0] = gbitmap_create_with_resource(RESOURCE_ID_SIGNAL_0_BLACK);
  signal_icons[1] = gbitmap_create_with_resource(RESOURCE_ID_SIGNAL_1_BLACK);
  signal_icons[2] = gbitmap_create_with_resource(RESOURCE_ID_SIGNAL_2_BLACK);
  signal_icons[3] = gbitmap_create_with_resource(RESOURCE_ID_SIGNAL_3_BLACK);
  
  // Create window
  window = window_create();
  
  // Set window handlers
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load
  });
  
  // Push window to stack
  window_stack_push(window, true);
  
  // Main event loop
  app_event_loop();
  
  // Destroy window
  window_destroy(window);
}
