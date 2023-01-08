#ifndef CONFIG_H
#define CONFIG_H

/* Modifier keys (Meta as default) */
#define MOD_KEY   XCB_MOD_MASK_4
#define MOD_SHIFT XCB_MOD_MASK_SHIFT

/* Window appearance */
#define BORDER_WIDTH 1
#define BORDER_COLOR_FOCUSED 0xffffff
#define BORDER_COLOR_UNFOCUSED 0xd0d0d0
/* Key bindings */
static const struct key keys[] = {
	/* MOD	     KEY       FUNC     ARG */
	{ MOD_KEY, XK_Return, shutdown, NULL },
};

#endif
