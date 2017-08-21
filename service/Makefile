TARGETS = janus-service

CFLAGS += -Wall -O6 `pkg-config --cflags --libs ncurses`
LDFLAGS += `pkg-config --libs ncurses`

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)

PREFIX ?= /usr/local

install: $(TARGETS)
	install -D -d $(DESTDIR)/$(PREFIX)/bin
	install -s -m 0755 $^ $(DESTDIR)/$(PREFIX)/bin
