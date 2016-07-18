.PHONY: clean
.SUFFIXES: .c .o

CFLAGS :=
CFLAGS += -Wall -Wextra
CFLAGS += -std=c99 -pedantic -g
CLFAGS += -pipe

LIBS=-lrt -lpthread

.c:
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<

TARGET :=
TARGET += spoke
TARGET += fan

all: $(TARGET)

clean:
	rm -f $(TARGET)
