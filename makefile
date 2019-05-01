# Makefile for Proxy Caching (Networks Final Project)
# Last updated: April 27, 2018
############## Variables ###############
CC = gcc

CFLAGS = -g -std=gnu99 -Wall -Wextra -Wfatal-errors -pedantic

LDFLAGS = -g 

LDLIBS =  -lm

INCLUDES = $(shell echo *.h)

#################### Rules #####################

all: hash_cache list_cache

# Compile step (.c files -> .o files)
%.o: %.c $(INCLUDES) $(CC) $(CFLAGS) -c $< -o $@
list_cache: server.o list_cache.o bandwidth.o
		$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
hash_cache: server.o hash_cache.o bandwidth.o
		$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
# Linking step ( .o -> executable programs ) 


clean: 
		rm -f hash_cache list_cache *.o
