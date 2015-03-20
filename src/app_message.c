#include <pebble.h>

#define NUM_MENU_SECTIONS 3
#define APP_VERSION 1

Window *window;
Window *message_item_window;
MenuLayer *menu_layer;
ScrollLayer *message_item_scroll_layer;
TextLayer *message_item_text_layer;
TextLayer *message_item_like_text_layer;
static GBitmap *icon_bitmap;

// Key values for AppMessage Dictionary
enum {
	MSGTYPE_KEY = 0,
	LATITUDE_KEY = 1,
	LONGTITUDE_KEY = 2,
	MESSAGE_KEY = 3,
	ID_KEY = 4,
	NUMBEROFLIKES_KEY = 5,
	NUMBEROFCOMMENTS_KEY = 6
};

typedef struct MessageItem {
	char *messageText;
	char *messageID;
	int numberOfLikes;
	int numberOfComments;
} MessageItem;

int counter = 0;
int messageArraySize;
bool readyToUpdate = true;
char *positionTitle;
char *lastUpdated;
MessageItem *messageItemArray;
int selectedMessageIndex;

char *numberLikesText;
char *numberCommentsText;

int latestOrAreaTop = 0; //0 is latest. 1 is area top

// Write message to buffer & send
void send_message(void){
	DictionaryIterator *iter;

	app_message_outbox_begin(&iter);
	dict_write_int32(iter, MSGTYPE_KEY, 0);
	dict_write_int32(iter, MESSAGE_KEY, latestOrAreaTop);

	dict_write_end(iter);
  app_message_outbox_send();
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *iter, void *context) {

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Received something");
	(void) context;
	Tuple *tuple;

  	readyToUpdate = false;
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "counter: %d", counter);
	tuple = dict_find(iter, MSGTYPE_KEY);
	if (tuple != NULL) {

    time_t currentTime;
		switch ((int)tuple->value->uint32) { //switch based on MSGTYPE
			case 0: //metadata. Signals data tranmission start. Has lat, long, message=size of list
				counter = 0;

				//get message size
				messageArraySize = (int)dict_find(iter, MESSAGE_KEY)->value->uint32;

				//deallocate old and alloc new message array
				if (messageItemArray) {
					int i;
					for (i = 0; i < messageArraySize; i++) {
						free(messageItemArray[i].messageText);
						free(messageItemArray[i].messageID);
					}
				}
				free(messageItemArray);
				messageItemArray = calloc(messageArraySize, sizeof(MessageItem));


				int latitude = (int)dict_find(iter, LATITUDE_KEY)->value->uint32;
				int longitude = (int)dict_find(iter, LONGTITUDE_KEY)->value->uint32;

				free(positionTitle);


				char *typeOfView;
				if (latestOrAreaTop == 0) {
					typeOfView = "Latest";
				} else {
					typeOfView = "Top";
				}


				APP_LOG(APP_LOG_LEVEL_DEBUG, "header size: %d", (int)(strlen(typeOfView) + sizeof(" - 00.000, 00.000") + 3));
				APP_LOG(APP_LOG_LEVEL_DEBUG, "header:%s - %d.%d, %d.%d", typeOfView, latitude / 1000, latitude - (latitude / 1000) * 1000, longitude / 1000, abs(longitude - (longitude / 1000) * 1000));


				positionTitle = calloc(strlen(typeOfView) + sizeof(" - 00.000, 00.000") + 3, sizeof(char));
				snprintf(positionTitle, strlen(typeOfView) + sizeof(" - 00.000, 00.000") + 3, "%s - %d.%d, %d.%d", typeOfView, latitude / 1000, latitude - (latitude / 1000) * 1000, longitude / 1000, longitude - (longitude / 1000) * 1000);


				APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Type 0 with size: %d", messageArraySize);
				break;

			case 1: //message item. Signals 1 message item. Has message text, id=messageID
				if (messageItemArray[counter].messageText != NULL)
					APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Type 1 but there is already something at index %d", counter);

				//allocate space for message test
				messageItemArray[counter].messageText = calloc(strlen(dict_find(iter, MESSAGE_KEY)->value->cstring) + 1, sizeof(char));

				//look for duplicate or extra transmission
				char *text = dict_find(iter, MESSAGE_KEY)->value->cstring; //the received message
				bool duplicate = false;

				if (counter == messageArraySize) { //we have a full message array
					APP_LOG(APP_LOG_LEVEL_DEBUG, "Type 1 menu full: %s", text);
					duplicate = true;
				}

				if (counter > 0) { //if duplicate message sent it will overflow mess item array
					if (strcmp(messageItemArray[counter - 1].messageText, text) == 0) { //received message is same as last inserted message in array
						APP_LOG(APP_LOG_LEVEL_DEBUG, "Type 1 with duplicate name: %s", text);
						duplicate = true;
					}
				}

				if (duplicate == false) { //if message is not duplicate and array has space
					//APP_LOG(APP_LOG_LEVEL_DEBUG, "Type 1 with name: %s", text);
					snprintf(messageItemArray[counter].messageText, strlen(text) + 1, "%s", text);
					messageItemArray[counter].numberOfLikes = (int)dict_find(iter, NUMBEROFLIKES_KEY)->value->int32;
					//APP_LOG(APP_LOG_LEVEL_DEBUG, "num like %d", dict_find(iter, NUMBEROFLIKES_KEY)->type);
					//APP_LOG(APP_LOG_LEVEL_DEBUG, "num comm %d", tmp->type);
					messageItemArray[counter].numberOfComments = (int)dict_find(iter, NUMBEROFCOMMENTS_KEY)->value->int32;
					counter++;
				}

				break;

				case 2: //metadata. Signals transmission done.
					currentTime = time(NULL);
					readyToUpdate = true;
					free(lastUpdated);
					lastUpdated = calloc(sizeof("Updated @ 00:00"), sizeof(char));
					strftime(lastUpdated, sizeof("Updated @ 00:00"), "Updated @ %H:%M", localtime(&currentTime));
					APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Type 2 done at %s", lastUpdated);
					break;

			default:
				APP_LOG(APP_LOG_LEVEL_DEBUG, "unknown message of type %d", (int)tuple->value->uint32);
				break;
		}

		//layer_mark_dirty((Layer*)menu_layer);
		menu_layer_reload_data(menu_layer);

	}

}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

static uint16_t num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	switch ((int)section_index) {
        case 0:
            return 1;
		case 1:
			return counter;
            return 1;
			break;
		case 2:
			return 0;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}

static uint16_t num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "num header");
	return NUM_MENU_SECTIONS;
}

static int16_t cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "header height %d", MENU_CELL_BASIC_HEADER_HEIGHT);
    if (cell_index->section == 0)
        return 40;
    else
        return 35;
}

static int16_t header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "header height %d", MENU_CELL_BASIC_HEADER_HEIGHT);
    if (section_index == 0)
        return 0;
    else
        return  MENU_CELL_BASIC_HEADER_HEIGHT;
}

void draw_row_callback(GContext *ctx, Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    char* test = "Unknown";
    char* appBanner = "Yebble";
    switch(cell_index->section) {
        case 0:
            menu_cell_basic_draw(ctx, cell_layer, appBanner, lastUpdated, icon_bitmap);
            break;
        case 1:
			graphics_context_set_text_color(ctx, GColorBlack);

			GRect textBounds = layer_get_bounds(cell_layer);
			textBounds.size.w -= 20;
			textBounds.size.h = 24;

			GRect likeTextBounds = layer_get_bounds(cell_layer);
			likeTextBounds.origin.x = likeTextBounds.size.w - 20;
			likeTextBounds.size.w = 20;

			APP_LOG(APP_LOG_LEVEL_DEBUG, "like bound %d %d %d %d", likeTextBounds.origin.x, likeTextBounds.origin.y, likeTextBounds.size.w, likeTextBounds.size.h);


			GRect commentsTextBounds = layer_get_bounds(cell_layer);
			commentsTextBounds.origin.x = 24;

			APP_LOG(APP_LOG_LEVEL_DEBUG, "draw row");

			char* likeText = calloc(messageItemArray[cell_index->row].numberOfLikes / 10 + 3, sizeof(char));
			snprintf(likeText, strlen(likeText), "%d", messageItemArray[cell_index->row].numberOfLikes);
			graphics_draw_text(ctx, likeText, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), likeTextBounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

			graphics_draw_text(ctx, messageItemArray[cell_index->row].messageText, fonts_get_system_font(FONT_KEY_GOTHIC_24), textBounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

			char* commentsText = calloc(messageItemArray[cell_index->row].numberOfComments / 10 + 2, sizeof(char));
			snprintf(commentsText, strlen(commentsText), "%d", messageItemArray[cell_index->row].numberOfComments);
			graphics_draw_text(ctx, commentsText, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), commentsTextBounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

			break;
        case 2:
            menu_cell_basic_draw(ctx, cell_layer, test, "", NULL);
            break;
        default:
            menu_cell_basic_draw(ctx, cell_layer, test, "", NULL);
            break;
    }

}

void draw_header_callback(GContext *ctx, Layer *cell_layer, uint16_t section_index, void *callback_context) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "draw header index %d", (int)section_index);
	char *appVersion = calloc(9 + APP_VERSION / 10 + 1, sizeof(char));
	switch((int)section_index) {
		case 0:
			break;
		case 1:
			menu_cell_basic_header_draw(ctx, cell_layer, positionTitle);
			break;
		case 2:
			snprintf(appVersion, 9 + APP_VERSION / 10 + 2, "Revision %d", APP_VERSION);
			menu_cell_basic_header_draw(ctx, cell_layer, appVersion);
			break;
    }
	free(appVersion);
}

void message_item_window_load(Window *window) {
	GRect bounds = layer_get_frame(window_get_root_layer(window));
    message_item_scroll_layer = scroll_layer_create(bounds);

    message_item_text_layer = text_layer_create(GRect(0, 0, 124, 2000));
	text_layer_set_font(message_item_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));

    text_layer_set_text(message_item_text_layer, messageItemArray[selectedMessageIndex].messageText);
	GSize max_size = text_layer_get_content_size(message_item_text_layer);
	text_layer_set_size(message_item_text_layer, GSize(max_size.w, max_size.h + 12));
	scroll_layer_set_content_size(message_item_scroll_layer, GSize(bounds.size.w, max_size.h + 12));

	APP_LOG(APP_LOG_LEVEL_DEBUG, "max size %d %d", max_size.w, max_size.h);

    text_layer_set_background_color(message_item_text_layer, GColorClear);
    text_layer_set_text_color(message_item_text_layer, GColorBlack);
    text_layer_set_text_alignment(message_item_text_layer, GTextAlignmentLeft);

    text_layer_set_overflow_mode(message_item_text_layer, GTextOverflowModeWordWrap);

	free(numberLikesText);
	numberLikesText = calloc(messageItemArray[selectedMessageIndex].numberOfLikes / 10 + 3, sizeof(char));
	snprintf(numberLikesText, strlen(numberLikesText), "%d", messageItemArray[selectedMessageIndex].numberOfLikes);

	//Like number text
	message_item_like_text_layer = text_layer_create(GRect(0, 0, 20, 30));
	text_layer_set_font(message_item_like_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text(message_item_like_text_layer,numberLikesText);
    text_layer_set_background_color(message_item_like_text_layer, GColorClear);
    text_layer_set_text_color(message_item_like_text_layer, GColorBlack);
    text_layer_set_text_alignment(message_item_like_text_layer, GTextAlignmentLeft);
    text_layer_set_overflow_mode(message_item_like_text_layer, GTextOverflowModeWordWrap);


    layer_add_child(window_get_root_layer(window), scroll_layer_get_layer(message_item_scroll_layer));

    scroll_layer_add_child(message_item_scroll_layer, text_layer_get_layer(message_item_text_layer));
	scroll_layer_add_child(message_item_scroll_layer, text_layer_get_layer(message_item_like_text_layer));


    scroll_layer_set_click_config_onto_window(message_item_scroll_layer, window);
}

void message_item_window_unload(Window *window) {
	text_layer_destroy(message_item_text_layer);
  scroll_layer_destroy(message_item_scroll_layer);
}

void select_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (cell_index->section == 0) {

	} else {
		selectedMessageIndex = cell_index->row;
		window_destroy(message_item_window);
		message_item_window = window_create();
		window_set_window_handlers(message_item_window, (WindowHandlers) {
			.load = message_item_window_load,
			.unload = message_item_window_unload
		});
		window_stack_push(message_item_window, true);
	}
}

void select_long_click_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	free(lastUpdated);
	lastUpdated = calloc(sizeof("Updating..."), sizeof(char));
	snprintf(lastUpdated, sizeof("Updating...") + 1, "%s", "Updating...");

	if (latestOrAreaTop == 0)
		latestOrAreaTop = 1;
	else
		latestOrAreaTop = 0;

	send_message();
}

void menu_window_load(Window *window) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "load menu");
  	icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON);

	free(lastUpdated);
	lastUpdated = calloc(sizeof("Updating..."), sizeof(char));
	snprintf(lastUpdated, sizeof("Updating...") + 1, "%s", "Updating...");

	positionTitle = calloc(2, sizeof(char));

	menu_layer = menu_layer_create(GRect(0, 0, 144, 168 - 16));

	menu_layer_set_click_config_onto_window(menu_layer, window);

	MenuLayerCallbacks callbacks = {
		.get_num_sections = (MenuLayerGetNumberOfSectionsCallback) num_sections_callback,
		.get_num_rows = (MenuLayerGetNumberOfRowsInSectionsCallback) num_rows_callback,
		.get_cell_height = (MenuLayerGetCellHeightCallback) cell_height_callback,
		.get_header_height = (MenuLayerGetHeaderHeightCallback) header_height_callback,
		.draw_row = (MenuLayerDrawRowCallback) draw_row_callback,
		.draw_header = (MenuLayerDrawHeaderCallback) draw_header_callback,
		.select_click = (MenuLayerSelectCallback) select_click_callback,
		.select_long_click = (MenuLayerSelectCallback) select_long_click_callback
	};
	menu_layer_set_callbacks(menu_layer, NULL, callbacks);

	layer_add_child(window_get_root_layer(window), menu_layer_get_layer(menu_layer));

	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

	send_message();
}

void menu_window_unload(Window *window) {
	menu_layer_destroy(menu_layer);
	gbitmap_destroy(icon_bitmap);

	if (messageItemArray) {
		int i;
		for (i = 0; i < messageArraySize; i++) {
			free(messageItemArray[i].messageText);
			free(messageItemArray[i].messageID);
		}
	}
	free(messageItemArray);
}

void init(void) {
	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
		.load = menu_window_load,
		.unload = menu_window_unload,
	});

	window_stack_push(window, true);
}

void deinit(void) {
	app_message_deregister_callbacks();
	window_destroy(window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}
