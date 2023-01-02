WFLAGS = -Wall -Wextra -Wshadow
CFLAGS = -std=c99 -pedantic ${WFLAGS}
LFLAGS = -lxcb -lxcb-keysyms

BINDIR = ${PREFIX}/bin
MANDIR = ${PREFIX}/share/man

SOURCE = ${wildcard *.c}

all: ${SOURCE}
	${CC} ${CFLAGS} -o $@ $< $(LFLAGS)

.PHONY: all
