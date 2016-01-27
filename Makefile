.PHONY: clean
.SUFFIXES: .c .o

CFLAGS=-Wall -Wextra -std=c99 -pedantic -g -pipe

LIBS=-lrt -lpthread

.c:
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<

TARGET :=
TARGET += test

all: $(TARGET)

clean:
	rm -f $(TARGET)
