#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "zwm.h"

/* Global variables */
xcb_connection_t *conn;
xcb_screen_t *screen;
int32_t mode; /* Mouse mode, refer to enum in header file. */

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

static void shutdown(void)
{
	xcb_disconnect(conn);
	fprintf(stdout, "Shutting down window manager.\n");
	exit(EXIT_SUCCESS);
}

/* Helper functions */
static void xcb_raise_window(xcb_drawable_t window)
{
	if (screen->root == window || !window) {
		return;
	}
	xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_STACK_MODE,
		XCB_STACK_MODE_ABOVE);
	/* xcb_flush(conn); */
}

static void xcb_focus_window(xcb_drawable_t window)
{
	if ((!window) || (window == screen->root)) {
		return;
	}
	xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window,
		XCB_CURRENT_TIME);
}

static void xcb_set_focus_color(xcb_window_t window, int32_t color)
{
	if ((BORDER_WIDTH <= 0) || (window == screen->root) || (!window)) {
		return;
	}
	xcb_change_window_attributes(conn, window, XCB_CW_BORDER_PIXEL, color);
	xcb_flush(conn);
}

static void xcb_get_keycodes(xcb_keysym_t keysym)
{
	/* Much love to @mcpcpc on Github, taken from his XWM source. */
	xcb_key_symbols_t *syms;
	xvb_keycode_t *keycode;

	syms = xcb_key_symbols_alloc(conn);
	keycode = (!(syms) ? NULL : xcb_key_symbols_get_keycode(syms, keysym));
	xcb_key_symbols_free(syms);
	return keycode;
}

/* Event functions */
static void button_press(xcb_generic_event_t *event) 
{ 
	xcb_button_press_event *button;
	xcb_drawable_t window;
	uint32_t mask;

	button = (xcb_button_press_event_t *) event;
	xcb_raise_window(button->child);
	/* Take control of pointer and confine it to root until release. */
	mask = XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION
		| XCB_EVENT_MASK_POINTER_MOTION_HINT;
	xcb_grab_pointer(conn, 0, screen->root, mask, XCB_GRAB_MODE_ASYNC, 
		XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE, XCB_CURRENT_TIME);
	/* xcb_flush(conn); */
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
	xcb_focus_window(enter->window);
}

static void focus_in(xcb_generic_event_t *event) 
{ 
	UNUSED(event); 
}

static void focus_out(xcb_generic_event_t *event) 
{ 
	UNUSED(event); 
}

static void map_request(xcb_generic_event_t *event) 
{ 
	xcb_map_request_event_t *map;
	uint32_t vals[5];

	map = (xcb_map_request_event_t *) event;
	xcb_map_window(conn, map->window);
	/* TODO: Replace w/ geom functions from xcb to get window size. */
	vals[0] = (screen->width_in_pixels / 2) - (800 / 2);
	vals[1] = (screen->height_in_pixels / 2) - (600 / 2);
	vals[2] = 800;
	vals[3] = 600;
	vals[4] = 1;
	xcb_configure_window(conn, map->window, XCB_CONFIG_WINDOW_X
		| XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH
		| XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH,
		vals);
	xcb_flush(conn);
	xcb_change_window_attributes_checked(conn, map->window, 
		XCB_CW_EVENT_MASK, XCB_EVENT_MASK_ENTER_WINDOW
		| XCB_EVENT_MASK_FOCUS_CHANGE);
	xcb_focus_window(map->window);
}

static void motion_notify(xcb_generic_event_t *event) 
{ 
	xcb_query_pointer_reply_t *mouse;
	xcb_get_geometry_cookie_t cookie;
	xcb_get_geometry_reply_t *geom;
	uint32_t values[2];

	UNUSED(event);
	if (!curr_window) {
		return;
	}
	/* Get mouse positions; */
	mouse = xcb_query_pointer_reply(conn, 
		xcb_query_pointer(conn, screen->root, 0));
	if (mouse == NULL) {
		die("Failed to get pointer position during motion event.\n");
	}
	/* Retrieve the geometry of the window we are handling. */
	cookie = xcb_get_geometry(conn, curr_window);
	geom = xcb_get_geometry_reply(conn, cookie, NULL);
	/* TODO: Rearrange code below w/ less indents and make it readable. */
	switch (mode) {
	case MOUSE_MODE_MOVE:
		uint16_t x;
		uint16_t y;

		x = geom->width + (2 * BORDER_WIDTH);
		y = geom->height + (2 * BORDER_WIDTH);
		values[0] = ((mouse->root_x + x) > screen->width_in_pixels)
			? (screen->width_in_pixels - x) : mouse->root_x;
		values[1] = ((mouse->root_y + y) > screen->height_in_pixels)
			? (screen->height_in_pixels - y) : mouse->root_y;
		xcb_configure_window(conn, curr_window, XCB_CONFIG_WINDOW_X
			| XCB_CONFIG_WINDOW_Y, values);
		break;
	case MOUSE_MODE_RESIZE:
		if (!(mouse->root_x <= geom->x) 
				|| !(mouse->root_y <= geom->y)) {
			values[0] = mouse->root_x - geom->x - BORDER_WIDTH;
			values[1] = mouse->root_y - geom->x - BORDER_WIDTH;
			if (values[0] >= WINDOW_MIN_WIDTH 
					&& values[1] >= WINDOW_MIN_HEIGHT) {
				xcb_configure_window(conn, curr_window, 
					XCB_CONFIG_WINDOW_WIDTH
					| XCB_CONFIG_WINDOW_HEIGHT, values);
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
	uint32_t values;
	size_t i;

	/* Assign events we want to know about for the main root window. */
	mask = XCB_CW_EVENT_MASK;
	values = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_PROPERTY_CHANGE;
	/* TODO: Add error checking for this next section w/ cookies. */
	xcb_change_window_attributes(conn, screen->root, mask, values);
	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
	/* Grab input for key bindings from the config. */
	for (i = 0; i < ARRAY_SIZE(keys); ++i) {
		xcb_keycode_t *keycode;

		keycode = xcb_get_keycodes(keys[i].keysym);
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
	while ((event = xcb_wait_for_event(conn))) {
		if (!event) {
			die("Error when handling X events.\n");
		}
		events[event->response_type & ~0x80] (&event);
		free(event);
		/* xcb_flush(conn); */
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
