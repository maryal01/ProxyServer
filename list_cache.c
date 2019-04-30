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

CacheBlock returnCacheBlock(CacheBlock cblock, char* url);
void removeLastAccessed(Cache cache);


Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    new_cache->total_elements = 0;
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

void insertToCache(Cache cache, char* url, char* content, int content_length){ 
    CacheBlock block = createCacheBlock();
    block->url = malloc(URL_SIZE);
    block->content = malloc(content_length);
    block->content_length = content_length;
    memcpy(block->url, url, URL_SIZE);
    memcpy(block->content, content, content_length);
    cache->cache[cache->total_elements] = block;
    cache->total_elements += 1;
 }


void removeLastAccessed(Cache cache){ 
    time_t last_accessed = time(NULL);
    int index = 0;
    for(int i = 0; i < cache->total_elements; i++){
        CacheBlock block = cache->cache[i];
        time_t block_access = block->last_accessed;
        if(block_access < last_accessed){
            last_accessed = block_access;
            index = i;
        }
    }
    cache->total_elements -= 1;
    CacheBlock block = cache->cache[index];
    cache->cache[index] = cache->cache[cache->total_elements];
    cache->cache[cache->total_elements] = NULL;
    free(block);
 }

char* getFromCache(Cache cache, char* url){   
    int total_elements = cache->total_elements;
    for(int i = 0; i < total_elements; i++){
        if( strcmp( cache->cache[i]->url, url) == 0 ){
            return cache->cache[i]->content;
        }
    }
    return NULL;
}

/* returns -1 if url not found */
int content_size(Cache cache, char* url)
{ 
    int total_elements = cache->total_elements;
    for(int i = 0; i < total_elements; i++){
        if( strcmp( cache->cache[i]->url, url) == 0 ){
            return cache->cache[i]->content_length;
        }
    }
    return -1;
}
