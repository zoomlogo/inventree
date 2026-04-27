# inventree - suckless inventory manager
# See COPYING file for copyright and license details.
include config.mk

SRC = inventree.c
OBJ = ${SRC:.c=.o}

all: inventree

.c.o:
	${CC} -c ${CFLAGS} $<

inventree: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f inventree ${OBJ}

# TODO install and uninstall

.PHONY: all clean
