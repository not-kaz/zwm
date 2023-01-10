#ifndef ZWM_H
#define ZWM_H

/* Helper macros */
#define UNUSED(_x_) (void)(_x_)
#define ARRAY_SIZE(_array_) (sizeof((_array_)) / sizeof((_array_)[0]))

/* Default window values */
#define WINDOW_WIDTH_DEFAULT 600
#define WINDOW_HEIGHT_DEFAULT 400
#define WINDOW_WIDTH_MIN 60
#define WINDOW_HEIGHT_MIN 40

/* Structures and enums */
enum mouse_button {
	MOUSE_BUTTON_NONE,
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_NITEMS
};

struct key {
	unsigned int mod;
	xcb_keysym_t keysym;
	void (*func)(char **com);
	char **com;
};

/* WM functions */
static void shutdown(char **com);
static void die(const char *fmt, ...);
/* Helper functions */
static void xcb_focus_window(xcb_drawable_t window); 
static void xcb_set_focus_color(xcb_window_t window, uint32_t color);
static xcb_keycode_t *xcb_get_keycode(xcb_keysym_t keysym);
static xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode);
static void xcb_raise_window(xcb_drawable_t window);
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

#endif
