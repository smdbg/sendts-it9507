CC ?= cc
PKG_CONFIG ?= pkg-config
PYTHON ?= python3
CFLAGS ?= -O2 -g
CFLAGS += -Wall -Wextra
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags libusb-1.0)
LDLIBS += $(shell $(PKG_CONFIG) --libs libusb-1.0)
PREFIX ?= /usr/local

.PHONY: all install clean test
OEM_OBJECTS = oem/IT9507.o oem/modulatorUser.o oem/eagleTuner.o oem/ADF4351.o oem_bridge.o
all: sendts-it9507
oem/%.o: oem/%.c $(wildcard oem/*.h)
	$(CC) $(CPPFLAGS) -O2 -g -ffunction-sections -fdata-sections -DURB_BUFSIZE_RX=65424 -c -o $@ $<
oem_bridge.o: oem_bridge.c oem_bridge.h $(wildcard oem/*.h)
	$(CC) $(CPPFLAGS) $(CFLAGS) -ffunction-sections -fdata-sections -c -o $@ $<
sendts-it9507: sendts-it9507.c firmware.h oem_bridge.h $(OEM_OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -Wl,--gc-sections -o $@ sendts-it9507.c $(OEM_OBJECTS) $(LDLIBS)

install: sendts-it9507
	install -D -m 0755 sendts-it9507 $(DESTDIR)$(PREFIX)/bin/sendts-it9507
	install -D -m 0644 LICENSE $(DESTDIR)$(PREFIX)/share/doc/sendts-it9507/LICENSE
	install -m 0644 README.md $(DESTDIR)$(PREFIX)/share/doc/sendts-it9507/README.md
	install -m 0644 CHANGELOG.md $(DESTDIR)$(PREFIX)/share/doc/sendts-it9507/CHANGELOG.md
	install -D -m 0644 contrib/99-it9507.rules $(DESTDIR)$(PREFIX)/share/doc/sendts-it9507/contrib/99-it9507.rules
	install -D -m 0644 oem/README.md $(DESTDIR)$(PREFIX)/share/doc/sendts-it9507/oem/README.md
	install -m 0644 oem/SOURCE.json $(DESTDIR)$(PREFIX)/share/doc/sendts-it9507/oem/SOURCE.json

test: sendts-it9507
	$(PYTHON) test_cli.py ./sendts-it9507
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -Wl,--gc-sections -o test-protocol test_protocol.c $(OEM_OBJECTS) $(LDLIBS)
	./test-protocol
	rm -f test-protocol

clean:
	rm -f sendts-it9507 test-protocol $(OEM_OBJECTS)
