/*
* Manish Aryal
* Proxy Cache
*/

#ifndef PROXY_CACHE
#define PROXY_CACHE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct T* Cache;

Cache createCache();

void insertToCache(Cache cache, char* url, char* content, int content_length);

char* getFromCache(Cache cache, char* url);

void removeLastAccessed();

int content_size(Cache cache, char* url);
#endif
