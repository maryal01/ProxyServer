/*
* Manish Aryal
* Proxy Cache
*/

#ifndef PROXY_CACHE
#define PROXY_CACHE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CACHE_SIZE 50
#define URL_SIZE 100

typedef struct T* Cache;

Cache createCache();

void insertToCache(Cache cache, char* url, char* content, int content_length);

char* getFromCache(Cache cache, char* url);

int content_size(Cache cache, char* url);
#endif
