#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#define CACHE_SIZE 50
#define URL_SIZE 100
#define CONTENT_SIZE 100

uint32_t jenkinsHashFunction(char *key);

typedef struct CacheBlock{
    char* url;
    char* content;
    time_t last_accessed;
    int port_number;
    CacheBlock next_element;
} *CacheBlock;

typedef struct Cache {
    CacheBlock cache[CACHE_SIZE];
    int total_elements;
    int last_accessed;
} *Cache;

Cache createCache(){
    return NULL;
}

void insertToCache(Cache cache, char* url, char* content){
    if(url == NULL || content == NULL || cache == NULL){
        assert("The parameters can not be null");}
    
    uint32_t hash_value = jenkinsHashFunction(url);
    uint32_t index = hash_value % CACHE_SIZE;

    CacheBlock block = malloc(sizeof(*block));
    block->url = malloc(URL_SIZE);
    block->content = malloc(CONTENT_SIZE);

    memcpy(block->url, url);
    memcpy(block->content, content);

    block->last_accessed = time(NULL);
    block->port_number = 80;
    block->next_element = NULL;
    if(cache->cache[index] == NULL){ 
        cache->cache[index] = block;
    } else {
        CacheBlock cblock  = cache->cache[index];
        while(cblock->next_element != NULL){
            cblock = cblock->next_element;
        }
        cblock->next_element = block;
    }

}

void removeLastAccessed(Cache cache){
    printf("Removing last accessed\n");
}

char* getFromCache(Cache cache, char* url){
    printf("getting the data from the cache\n");
    return "hacked";
}

void resizeCache(Cache *cache){
    printf("resizing the cache\n");
}

uint32_t jenkinsHashFunction(char *key)
{
    uint32_t hash = 0;
    uint32_t i = 0;
    char character = key[i];
    while(character != '\0'){
        i++;
        hash += character;
        hash += (hash << 10);
        hash ^= (hash >> 6);
        character = key[i];
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

int main(int argc, char** argv){
    Cache cache = malloc(sizeof(*cache));

    insertToCache(cache, "https://www.facebook.com", "hacked\n");
    char* content = getFromCache(cache, "https://www.facebook.com");
    removeLastAccessed(cache);
    return 0;
}

