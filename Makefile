# inventree - suckless inventory manager
# See COPYING file for copyright and license details.
include config.mk

SRC = inventree.c
OBJ = ${SRC:.c=.o}

all: inventree

.c.o:
	${CC} -c ${CFLAGS} $<

$(OBJ): config.mk

inventree: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f inventree ${OBJ}

dist: clean
	mkdir -p inventree-$(VERSION)
	cp -R config.mk COPYING inventree.1 Makefile README $(SRC) inventree-$(VERSION)
	tar -cf - inventree-$(VERSION) | gzip >inventree-$(VERSION).tar.gz
	rm -rf inventree-$(VERSION)

install: inventree
	mkdir -p $(PREFIX)/bin
	cp -f inventree $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/inventree
	mkdir -p $(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" <inventree.1 >$(MANPREFIX)/man1/inventree.1
	chmod 644 $(PREFIX)/man1/inventree.1

uninstall:
	rm -f $(PREFIX)/bin/inventree
	rm -f $(PREFIX)/man1/inventree.1

.PHONY: all clean dist install uninstall
