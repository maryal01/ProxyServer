CC = gcc -O3

CFLAGS = -g -std=gnu99 -Wall -Wextra -Wfatal-errors -pedantic $(IFLAGS)

INCLUDES = $(shell echo *.h)

all: hash_cache list_cache

%.o: %.c $(INCLUDES)
        $(CC) $(CFLAGS) -c $< -o $@

hash_cache: proxy.o list_cache.o
        $(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
list_cache: proxy.o list_cache.o
        $(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

clean:
        rm -f hash_cache list_cache *.o
