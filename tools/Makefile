CC       ?= cc
CFLAGS   ?= -O2
WARNINGS := -std=c11 -Wall -Wextra -Wformat=2 -Wpedantic
PREFIX   ?= /usr/local
BINDIR   := $(PREFIX)/bin

TARGET := wrong8007ctl

.PHONY: all install uninstall clean

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(WARNINGS) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)
