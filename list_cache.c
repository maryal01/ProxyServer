#include <time.h>
#include <string.h>
#include <assert.h>

#include "cache.h"
#define CACHE_SIZE 50
#define URL_SIZE 100


typedef struct CacheBlock{
    char* url;
    char* content;
    int content_length;
    time_t last_accessed;
} *CacheBlock;

struct T {
    CacheBlock cache[CACHE_SIZE];
    int total_elements;
};


CacheBlock returnCacheBlock(CacheBlock cache, char* url);
CacheBlock returnCacheBlock(CacheBlock cblock, char* url);
void removeLastAccessed(Cache cache);


Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    for(int i = 0; i < CACHE_SIZE; i++){
        new_cache->cache[i] = NULL;
    }
    return new_cache;

}

CacheBlock createCacheBlock(){
    CacheBlock block = malloc(sizeof(*block));
    block->last_accessed = time(NULL);
    return block;
}

void insertToCache(Cache cache, char* url, char* content, int content_length){ return; }

CacheBlock returnCacheBlock(CacheBlock cblock, char* url){ return NULL; }


void removeLastAccessed(Cache cache){ return; }

void updateRemovalList(CacheBlock cblock){ return; }

char* getFromCache(Cache cache, char* url){    return NULL; }

/* returns -1 if url not found */
int content_size(Cache cache, char* url)
{ return -1; }