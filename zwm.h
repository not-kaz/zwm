#ifndef ZWM_H
#define ZWM_H

/* Helper macros */
#define UNUSED(_x_) (void)(_x_)
#define ARRAY_SIZE(_array_) (sizeof((_array_)) / sizeof((_array_)[0]))

enum mouse_mode = {
	MOUSE_MODE_MOVE,
	MOUSE_MODE_RESIZE,
	MOUSE_MODE_NITEMS
};

/* Structures */
struct key {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*func)(char **com);
	char **com;
};

/* WM functions */
static void shutdown(void);
static void die(const char *fmt, ...);
/* Helper functions */
static void xcb_raise_window(xcb_drawable_t window);
static xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t keysym);
static void xcb_focus_window(xcb_drawable_t window); 
static void xcb_set_focus_color(xcb_window_t window, int32_t color);
/* Event functions */
static void button_press(xcb_generic_event_t *event);
static void button_release(xcb_generic_event_t *event);
static void destroy_notify(xcb_generic_event_t *event);
static void enter_notify(xcb_generic_event_t *event);
static void focus_in(xcb_generic_event_t *event);
static void focus_out(xcb_generic_event_t *event);
static void key_press(xcb_generic_event_t *event);
static void map_request(xcb_generic_event_t *event);
static void motion_notify(xcb_generic_event_t *event);
/* Internal functions */
static void setup(void);
static void run(void);

/* X events to handle */
static void (*events[])(xcb_generic_event_t *event) = {
	[XCB_BUTTON_PRESS] = button_press,
	[XCB_BUTTON_RELEASE] = button_release,
	[XCB_DESTROY_NOTIFY] = destroy_notify,
	[XCB_ENTER_NOTIFY] = enter_notify,
	[XCB_FOCUS_IN] = focus_in,
	[XCB_FOCUS_OUT] = focus_out,
	[XCB_KEY_PRESS] = key_press,
	[XCB_MAP_REQUEST] = map_request,
	[XCB_MOTION_NOTIFY] = motion_notify,
	/*[XCB_NONE] = NULL*/
};

#endif
