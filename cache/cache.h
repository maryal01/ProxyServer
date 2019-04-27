/*
* Manish Aryal
* Proxy Cache
*/

#ifndef PROXY_CACHE
#define PROXY_CACHE

#include <stdlib.h>
#include <stdint.h>

typedef struct T* Cache;

Cache createCache();

void insertToCache(Cache cache, char* url, char* content);

char* getFromCache(Cache cache, char* url);


#endif