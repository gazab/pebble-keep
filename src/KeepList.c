#include <pebble.h>
#include "KeepList.h"

#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ICONS 2
#define NUM_FIRST_MENU_ITEMS 3
#define NUM_SECOND_MENU_ITEMS 1 
  
static Window *window;

char listItems[50][20];
uint8_t numOfListItems = 0;

// This is a menu layer
// You have more control than with a simple menu layer
static MenuLayer *menu_layer;

// Menu items can optionally have an icon drawn with them
static GBitmap *menu_icons[NUM_MENU_ICONS];

static int current_icon = 0;

// You can draw arbitrary things in a menu item such as a background
static GBitmap *menu_background;

void listnote_part_received(DictionaryIterator *received) {
  // Note index
  uint8_t index = dict_find(received, 1)->value->uint8;
  // Number of list items
	numOfListItems = dict_find(received, 2)->value->uint8;
  
  bool listFinished = false;
  
  // Start reading list items
  for (uint8_t i = 0; i < 3; i++)
	{
		uint8_t listPos = index + i;
		if (listPos > numOfListItems - 1)
		{
			listFinished = true;
			break;
		}

		strcpy(listItems[listPos], dict_find(received, i + 3)->value->cstring);
	}
  
  // If we only have 3 items we are finished
  if (index + 3 == numOfListItems)
		listFinished = true;

	if (listFinished)
	{
		//loading = false;
  }
  	else
	{
    // Request more list items
		DictionaryIterator *iterator;
		app_message_outbox_begin(&iterator);

		dict_write_uint8(iterator, 0, 3);
		dict_write_uint8(iterator, 1, index + 3);

		app_message_outbox_send();

		app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
	}
  
  layer_set_hidden((Layer*) menu_layer, false);
  menu_layer_reload_data(menu_layer);
}


// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t list_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t list_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return numOfListItems;

    case 1:
      return NUM_SECOND_MENU_ITEMS;

    default:
      return 0;
  }
}

int16_t list_get_row_height_callback(MenuLayer *me,  MenuIndex *cell_index, void *data) {
	return 30;
}

// A callback is used to specify the height of the section header
static int16_t list_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // This is a define provided in pebble.h that you may use for the default height
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is
static void list_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Determine which section we're working with
  switch (section_index) {
    case 0:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, "Unchecked");
      break;

    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "Checked");
      break;
  }
}


int is_checked(char *item) {
  if(item[0] == '+')
    return 1;
  else
    return 0;
}

// This is the menu item draw callback where you specify what each item should look like
static void list_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {  
  	graphics_context_set_text_color(ctx, GColorBlack);
  if(is_checked(listItems[cell_index->row])) {
    graphics_draw_bitmap_in_rect(ctx, menu_icons[1], GRect(3, 7, 16, 16));
  }
  else {
    graphics_draw_bitmap_in_rect(ctx, menu_icons[0], GRect(3, 7, 16, 16));
  }
	graphics_draw_text(ctx, listItems[cell_index->row]+2, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(24, -2, 124, 30), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

// Here we capture when a user selects a menu item
void list_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  switch (cell_index->row) {
    // This is the menu item with the cycling icon
    case 1:
      // Cycle the icon
      current_icon = (current_icon + 1) % NUM_MENU_ICONS;
      // After changing the icon, mark the layer to have it updated
      layer_mark_dirty(menu_layer_get_layer(menu_layer));
      break;
  }

}

// This initializes the menu upon window load
void list_window_load(Window *window) {
  // Here we load the bitmap assets
  // resource_init_current_app must be called before all asset loading
  int num_menu_icons = 0;
  menu_icons[num_menu_icons++] = gbitmap_create_with_resource(RESOURCE_ID_UNCHECKED);
  menu_icons[num_menu_icons++] = gbitmap_create_with_resource(RESOURCE_ID_CHECKED);

  // And also load the background
  //menu_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_BRAINS);

  // Now we prepare to initialize the menu layer
  // We need the bounds to specify the menu layer's viewport size
  // In this case, it'll be the same as the window's
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  menu_layer = menu_layer_create(bounds);
  
  // Hide it
  layer_set_hidden((Layer*) menu_layer, true);

  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = list_get_num_sections_callback,
    .get_num_rows = list_get_num_rows_callback,
    .get_cell_height = list_get_row_height_callback,
    .get_header_height = list_get_header_height_callback,
    .draw_header = list_draw_header_callback,
    .draw_row = list_draw_row_callback,
    .select_click = list_menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(menu_layer, window);

  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

void list_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(menu_layer);

  // Cleanup the menu icons
  for (int i = 0; i < NUM_MENU_ICONS; i++) {
    gbitmap_destroy(menu_icons[i]);
  }
}

void list_init() {
  window = window_create();

  // Setup the window handlers
  window_set_window_handlers(window, (WindowHandlers) {
    .load = list_window_load,
    .unload = list_window_unload,
  });

  window_stack_push(window, true /* Animated */);
}
