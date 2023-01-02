#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "zwm.h"

/* Global variables */
xcb_connection_t *conn;
xcb_screen_t *screen;

/* Included here to allow access to variables located above. */
#include "config.h"

/* Event functions */
static void button_press(xcb_generic_event_t *event) { UNUSED(event); }
static void button_release(xcb_generic_event_t *event) { UNUSED(event); }
static void destroy_notify(xcb_generic_event_t *event) { UNUSED(event); }
static void enter_notify(xcb_generic_event_t *event) { UNUSED(event); }
static void focus_in(xcb_generic_event_t *event) { UNUSED(event); }
static void focus_out(xcb_generic_event_t *event) { UNUSED(event); }
static void map_request(xcb_generic_event_t *event) { UNUSED(event); }
static void motion_notify(xcb_generic_event_t *event) { UNUSED(event); }

/* WM functions */
static void shutdown(void)
{
	xcb_disconnect(conn);
	fprintf(stdout, "Shutting down window manager.\n");
	exit(EXIT_SUCCESS);
}

/* Helper functions */
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

/* Internal functions */
static void die(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	xcb_disconnect(conn);
	exit(EXIT_FAILURE);
}

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
