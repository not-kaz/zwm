#ifndef ZWM_H
#define ZWM_H

/* Helper macros */
#define UNUSED(_x_) (void)(_x_)
#define ARRAY_SIZE(_array_) (sizeof((_array_)) / sizeof((_array_)[0]))

/* Structures */
struct key {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*func)(char **com);
	char **com;
};

/* Event functions */
static void button_press(xcb_generic_event_t *event);
static void button_release(xcb_generic_event_t *event);
static void destroy_notify(xcb_generic_event_t *event);
static void enter_notify(xcb_generic_event_t *event);
static void focus_in(xcb_generic_event_t *event);
static void focus_out(xcb_generic_event_t *event);
static void map_request(xcb_generic_event_t *event);
static void motion_notify(xcb_generic_event_t *event);
/* WM functions */
static void shutdown(void);
/* Helper functions */
static xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t keysym);
/* Internal functions */
static void die(const char *fmt, ...);
static void setup(void);
static void run(void);

/* X events to handle */
static void (*events[])(xcb_generic_event_t *event) = {
	[XCB_NONE] = NULL
};

#endif
