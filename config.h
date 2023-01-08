#ifndef CONFIG_H
#define CONFIG_H

/* Modifier keys (Meta as default) */
#define MOD_KEY   XCB_MOD_MASK_4
#define MOD_SHIFT XCB_MOD_MASK_SHIFT

/* Key bindings */
static const struct key keys[] = {
	/* MOD	     KEY       FUNC     ARG */
	{ MOD_KEY, XK_Return, shutdown, NULL },
};

#endif
