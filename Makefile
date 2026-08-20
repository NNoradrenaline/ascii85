TARGET = ascii85
SOURCE_PARTS := $(wildcard src/ascii85_part_*.inc)
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wconversion -Wshadow -pedantic -O2
LDFLAGS ?=
RM ?= rm -f
PYTHON ?= python3
FUZZ_CC ?= clang
AFL_CC ?= afl-clang-fast
PREFIX ?= /usr/local
DESTDIR ?=
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1
INSTALL ?= install

.PHONY: all clean test sanitize fuzz fuzz-smoke fuzz-process fuzz-afl install uninstall format

all: $(TARGET)

$(TARGET): ascii85.c $(SOURCE_PARTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

test: $(TARGET)
	$(PYTHON) tests.py ./$(TARGET)

sanitize:
	$(CC) -std=c11 -Wall -Wextra -pedantic -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		-o $(TARGET)-san ascii85.c
	ASAN_OPTIONS=detect_leaks=1 $(PYTHON) tests.py ./$(TARGET)-san

fuzz:
	$(FUZZ_CC) -std=c11 -O1 -g -fno-omit-frame-pointer \
		-fsanitize=fuzzer,address,undefined -o fuzz_a85x fuzz_a85x.c

fuzz-smoke: fuzz
	@tmp=$$(mktemp -d); \
	trap 'rm -rf "$$tmp"' EXIT INT TERM; \
	cp fuzz/corpus/* "$$tmp"/; \
	./fuzz_a85x "$$tmp" -max_total_time=10 -timeout=3 -max_len=1024 -rss_limit_mb=512

fuzz-process: $(TARGET)
	$(PYTHON) fuzz/fuzz_smoke.py ./$(TARGET)

fuzz-afl:
	$(AFL_CC) -std=c11 -Wall -Wextra -O1 -g -o $(TARGET)-afl ascii85.c
	@echo "Run: afl-fuzz -i fuzz/corpus -o fuzz/findings -- ./$(TARGET)-afl -x -d"

install: $(TARGET)
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)" "$(DESTDIR)$(MANDIR)"
	$(INSTALL) -m 755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"
	$(INSTALL) -m 644 ascii85.1 "$(DESTDIR)$(MANDIR)/ascii85.1"

uninstall:
	$(RM) "$(DESTDIR)$(BINDIR)/$(TARGET)" "$(DESTDIR)$(MANDIR)/ascii85.1"

format:
	clang-format -i ascii85.c fuzz_a85x.c

clean:
	$(RM) $(TARGET) $(TARGET).exe $(TARGET)-san $(TARGET)-afl fuzz_a85x fuzz_a85x.exe
