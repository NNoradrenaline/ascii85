TARGET = ascii85

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wconversion -Wshadow -pedantic -O2
LDFLAGS ?=
RM ?= rm -f
PYTHON ?= python3

.PHONY: all clean test sanitize

all: $(TARGET)

$(TARGET): ascii85.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

test: $(TARGET)
	$(PYTHON) tests.py ./$(TARGET)

sanitize:
	$(CC) -std=c11 -Wall -Wextra -pedantic -O1 -g \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		-o $(TARGET)-san ascii85.c
	$(PYTHON) tests.py ./$(TARGET)-san

clean:
	$(RM) $(TARGET) $(TARGET).exe $(TARGET)-san
