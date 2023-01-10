#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "zwm.h"

/* Global variables */
xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_drawable_t curr_window;
int32_t mouse; /* Mouse state, refer to enum in header file. */

/* Included here to allow access to variables located above. */
#include "config.h"

/* WM functions */
static void die(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	xcb_disconnect(conn);
	exit(EXIT_FAILURE);
}

static void shutdown(char **com)
{
	xcb_disconnect(conn);
	fprintf(stdout, "Shutting down window manager.\n");
	exit(EXIT_SUCCESS);
}

/* Helper functions */
static void xcb_focus_window(xcb_drawable_t window)
{
	if ((!window) || (window == screen->root)) {
		return;
	}
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window,
		XCB_CURRENT_TIME);
}

static void xcb_set_focus_color(xcb_window_t window, uint32_t color)
{
	uint32_t val[1];

	if ((BORDER_WIDTH <= 0) || (window == screen->root) || (!window)) {
		return;
	}
	val[0] = color;
	xcb_change_window_attributes(conn, window, XCB_CW_BORDER_PIXEL, val);
	xcb_flush(conn);
}

static xcb_keycode_t *xcb_get_keycode(xcb_keysym_t keysym)
{
	/* Much love to @mcpcpc on Github, taken from his XWM source. */
	xcb_key_symbols_t *syms;
	xcb_keycode_t *keycode;

	syms = xcb_key_symbols_alloc(conn);
	keycode = (!(syms) ? NULL : xcb_key_symbols_get_keycode(syms, keysym));
	xcb_key_symbols_free(syms);
	return keycode;
}

static xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode)
{
	/* Much love to @mcpcpc on Github, taken from his XWM source. */
	xcb_key_symbols_t *syms;
	xcb_keysym_t keysym;

	syms = xcb_key_symbols_alloc(conn);
	keysym = (!(syms) ? 0 : xcb_key_symbols_get_keysym(syms, keycode, 0));
	xcb_key_symbols_free(syms);
	return keysym;
}

static void xcb_raise_window(xcb_drawable_t window)
{
	uint32_t val[1];

	if ((!window) || (window == screen->root)) {
		return;
	}
	val[0] = XCB_STACK_MODE_ABOVE;
	xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_STACK_MODE, val);
	xcb_flush(conn);
}

/* Event functions */
static void button_press(xcb_generic_event_t *event) 
{ 
	xcb_button_press_event_t *button;
	uint32_t mask;

	button = (xcb_button_press_event_t *) event;
	curr_window = button->child;
	xcb_raise_window(curr_window);
	mouse = (button->detail == MOUSE_BUTTON_LEFT) ? MOUSE_BUTTON_LEFT 
		: (curr_window) ? MOUSE_BUTTON_RIGHT : MOUSE_BUTTON_NONE;
	/* Take control of pointer and confine it to root until release. */
	mask = XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION
		| XCB_EVENT_MASK_POINTER_MOTION_HINT;
	xcb_grab_pointer(conn, 0, screen->root, mask, XCB_GRAB_MODE_ASYNC, 
		XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, XCB_CURRENT_TIME);
	xcb_flush(conn);
}

static void button_release(xcb_generic_event_t *event) 
{ 
	UNUSED(event); 
	xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
}

static void destroy_notify(xcb_generic_event_t *event) 
{ 
	xcb_destroy_notify_event_t *destroy;	

	destroy = (xcb_destroy_notify_event_t *) event;
	xcb_kill_client(conn, destroy->window);
}

static void enter_notify(xcb_generic_event_t *event) 
{ 
	xcb_enter_notify_event_t *enter;

	enter = (xcb_enter_notify_event_t *) event;
	xcb_focus_window(enter->event);
}

static void focus_in(xcb_generic_event_t *event) 
{ 
	xcb_focus_in_event_t *focus;

	focus = (xcb_focus_in_event_t *) event;
	xcb_set_focus_color(focus->event, BORDER_COLOR_FOCUSED);
}

static void focus_out(xcb_generic_event_t *event) 
{ 
	xcb_focus_in_event_t *focus;

	focus = (xcb_focus_in_event_t *) event;
	xcb_set_focus_color(focus->event, BORDER_COLOR_UNFOCUSED);
}

static void key_press(xcb_generic_event_t *event)
{
	xcb_key_press_event_t *key;
	xcb_keysym_t keysym;
	size_t i;

	key = (xcb_key_press_event_t *) event;
	keysym = xcb_get_keysym(key->detail);
	curr_window = key->child;
	for (i = 0; i < ARRAY_SIZE(keys); ++i) {
		if ((keys[i].keysym == keysym)
				&& (keys[i].mod == key->state)) {
			keys[i].func(keys[i].com);
		}
	}
}

static void map_request(xcb_generic_event_t *event) 
{ 
	xcb_map_request_event_t *map;
	uint32_t vals[5];

	map = (xcb_map_request_event_t *) event;
	xcb_map_window(conn, map->window);
	/* TODO: Replace w/ geom functions from xcb to get window size. */
	vals[0] = (screen->width_in_pixels / 2) - (WINDOW_WIDTH_DEFAULT / 2);
	vals[1] = (screen->height_in_pixels / 2) - (WINDOW_HEIGHT_DEFAULT/ 2);
	vals[2] = WINDOW_WIDTH_DEFAULT;
	vals[3] = WINDOW_HEIGHT_DEFAULT;
	vals[4] = BORDER_WIDTH;
	xcb_configure_window(conn, map->window, XCB_CONFIG_WINDOW_X
		| XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH
		| XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH,
		vals);
	xcb_flush(conn);
	vals[0] = XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE;
	xcb_change_window_attributes_checked(conn, map->window, 
		XCB_CW_EVENT_MASK, vals); 
	xcb_focus_window(map->window);
}

static void motion_notify(xcb_generic_event_t *event) 
{ 
	xcb_query_pointer_cookie_t pointer_cookie;
	xcb_query_pointer_reply_t *pointer;
	xcb_get_geometry_cookie_t geom_cookie;
	xcb_get_geometry_reply_t *geom;
	int16_t x;
	int16_t y;
	uint32_t vals[2];

	UNUSED(event);
	if (!curr_window) {
		return;
	}
	/* Query mouse data; */
	pointer_cookie = xcb_query_pointer(conn, screen->root);
	pointer = xcb_query_pointer_reply(conn, pointer_cookie, 0);
	if (pointer == NULL) {
		return;
	}
	/* TODO: Rearrange code below w/ less indents and make it readable. */
	switch (mouse) {
	case MOUSE_BUTTON_LEFT:
		geom_cookie = xcb_get_geometry(conn, curr_window);
		geom = xcb_get_geometry_reply(conn, geom_cookie, NULL);
		if (geom == NULL) {
			return;
		}
		x = geom->width + (2 * BORDER_WIDTH);
		y = geom->height + (2 * BORDER_WIDTH);
		vals[0] = ((pointer->root_x + x) > screen->width_in_pixels)
			? (screen->width_in_pixels - x) : pointer->root_x;
		vals[1] = ((pointer->root_y + y) > screen->height_in_pixels)
			? (screen->height_in_pixels - y) : pointer->root_y;
		xcb_configure_window(conn, curr_window, XCB_CONFIG_WINDOW_X
			| XCB_CONFIG_WINDOW_Y, vals);
		break;
	case MOUSE_BUTTON_RIGHT:
		geom_cookie = xcb_get_geometry(conn, curr_window);
		geom = xcb_get_geometry_reply(conn, geom_cookie, NULL);
		if (geom == NULL) {
			return;
		}
		if (!((pointer->root_x <= geom->x)
				|| (pointer->root_y <= geom->y))) {
			vals[0] = pointer->root_x - geom->x - BORDER_WIDTH;
			vals[1] = pointer->root_y - geom->x - BORDER_WIDTH;
			if ((vals[0] >= WINDOW_WIDTH_MIN)
					&& (vals[1] >= WINDOW_HEIGHT_MIN)) {
				xcb_configure_window(conn, curr_window, 
					XCB_CONFIG_WINDOW_WIDTH
					| XCB_CONFIG_WINDOW_HEIGHT, vals);
			}
		}
		break;
	default:
		break;
	}
}

/* Internal functions */
static void setup(void)
{
	uint32_t mask;
	uint32_t vals[1];
	size_t i;

	/* Assign events we want to know about for the main root window. */
	mask = XCB_CW_EVENT_MASK;
	vals[0] = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE;
	/* TODO: Add error checking for this next section w/ cookies. */
	xcb_change_window_attributes(conn, screen->root, mask, vals);
	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
	/* Grab input for key bindings from the config. */
	for (i = 0; i < ARRAY_SIZE(keys); ++i) {
		xcb_keycode_t *keycode;

		keycode = xcb_get_keycode(keys[i].keysym);
		if (!keycode) {
			continue;
		}
		xcb_grab_key(conn, 1, screen->root, keys[i].mod, *keycode,
			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
	}
	xcb_flush(conn);
	/* Grab primary mouse buttons to main root window. */
	xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
		XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, 1, MOD_KEY);
	xcb_grab_button(conn, 0, screen->root, XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
		XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, 3, MOD_KEY);
	xcb_flush(conn);
}

static void run(void)
{
	xcb_generic_event_t *event;

	/* Handle events and keeps the program running. */
	while (1 && (event = xcb_wait_for_event(conn))) {
		if (!event) {
			if (xcb_connection_has_error(conn)) {
				die("Polling for events failed.\n");
			}
			continue;
		}
		switch (event->response_type & ~0x80) {
		case XCB_BUTTON_PRESS:
			button_press(event);
			break;
		case XCB_BUTTON_RELEASE:
			button_release(event);
			break;
		case XCB_DESTROY_NOTIFY:
			destroy_notify(event);
			break;
		case XCB_ENTER_NOTIFY:
			enter_notify(event);
			break;
		case XCB_FOCUS_IN:
			focus_in(event);
			break;
		case XCB_FOCUS_OUT:
			focus_out(event);
			break;
		case XCB_KEY_PRESS:
			key_press(event);
			break;
		case XCB_MAP_REQUEST:
			map_request(event);
			break;
		case XCB_MOTION_NOTIFY:
			motion_notify(event);
			break;
		}
		free(event);
	}
}

int main(void)
{
	/* Establish connection to the X server. */
	conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		die("Failed to make a connection to X server.\n");
	}
	/* Retrieve screen data to work with. */
	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	if (!screen) {
		die("Failed to get screen data.\n");
	}
	setup();
	run();
	return 0;
}
