TARGET = ascii85

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wconversion -Wshadow -pedantic -O2
LDFLAGS ?=
RM ?= rm -f
PYTHON ?= python3
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1
DESTDIR ?=
AFL_CC ?= afl-clang-fast

.PHONY: all clean test sanitize install uninstall fuzz-afl fuzz-smoke

all: $(TARGET)

$(TARGET): ascii85.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

test: $(TARGET)
	$(PYTHON) tests.py ./$(TARGET)

sanitize:
	$(CC) -std=c11 -Wall -Wextra -Wconversion -Wshadow -pedantic -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		-o $(TARGET)-san ascii85.c
	$(PYTHON) tests.py ./$(TARGET)-san

fuzz-smoke: $(TARGET)
	$(PYTHON) fuzz/fuzz_smoke.py ./$(TARGET)

fuzz-afl:
	$(AFL_CC) -std=c11 -Wall -Wextra -O1 -g -o $(TARGET)-afl ascii85.c
	@echo "Run: afl-fuzz -i fuzz/corpus -o fuzz/findings -- ./$(TARGET)-afl -x -d"

install: $(TARGET)
	install -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(MANDIR)"
	install -m 0755 "$(TARGET)" "$(DESTDIR)$(BINDIR)/$(TARGET)"
	install -m 0644 ascii85.1 "$(DESTDIR)$(MANDIR)/ascii85.1"

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/$(TARGET)" "$(DESTDIR)$(MANDIR)/ascii85.1"

clean:
	$(RM) $(TARGET) $(TARGET).exe $(TARGET)-san $(TARGET)-afl
