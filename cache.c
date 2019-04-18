#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define CACHE_SIZE 2
#define URL_SIZE 100
#define CONTENT_SIZE 1024

uint32_t jenkinsHashFunction(char *key);

typedef struct CacheBlock{
    char* url;
    char* content;
    int content_length;
    time_t last_accessed;
    struct CacheBlock* next_element;
} *CacheBlock;

typedef struct Cache {
    CacheBlock cache[CACHE_SIZE];
    int total_elements;
} *Cache;

Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    for(int i = 0; i < CACHE_SIZE; i++){
        new_cache->cache[i] = NULL;
    }
    return new_cache;

}
CacheBlock returnCacheBlock(CacheBlock cache, char* url);
char* getFromCache(Cache cache, char* url);
void insertToCache(Cache cache, char* url, char* content, int content_length){

    if(url == NULL || content == NULL || cache == NULL){
        assert("The parameters can not be null");}
    CacheBlock block = malloc(sizeof(*block));
    block->url = malloc(URL_SIZE);
    block->content = malloc(content_length);
    block->content_length = content_length;
    memcpy(block->url, url, URL_SIZE);
    memcpy(block->content, content, content_length);
    block->last_accessed = time(NULL);
    block->next_element = NULL;

    uint32_t hash_value = jenkinsHashFunction(url);
    uint32_t index = hash_value % CACHE_SIZE;
    fprintf(stderr, "%s url is inserted at index %d \n", url, index);
    CacheBlock current_block = cache->cache[index];
    if(current_block == NULL){
        cache->cache[index] = block;
    } else {
        CacheBlock cblock = returnCacheBlock(current_block, url);
        if( strcmp(cblock->url, url) == 0){
            free(block);
            int total_size = content_length + cblock->content_length;
            char* total_content = malloc(total_size);
            memcpy(total_content, cblock->content, cblock->content_length);
            fprintf(stderr, "Copied %s of length %d \n", cblock->content, cblock->content_length);
            memcpy(total_content + cblock->content_length, content, content_length);
            fprintf(stderr, "Copied %s of length %d\n", content, content_length);
            free(cblock->content);
            cblock->content = malloc(total_size);
            cblock->content_length = total_size;
            memcpy(cblock->content, total_content, total_size);
            fprintf(stderr, "Copied %s of length %d\n", total_content, total_size);
        } else {
            cblock->next_element = block;
        }
    }

}

CacheBlock returnCacheBlock(CacheBlock cblock, char* url){
    CacheBlock pblock = NULL;
    while(cblock != NULL && strcmp(cblock->url, url) != 0){
        pblock = cblock;
        cblock = cblock->next_element;
    }
    if(cblock == NULL){
        return pblock;
    }
    return cblock;
}


void removeLastAccessed(Cache cache){
    printf("Removing last accessed\n");
}

char* getFromCache(Cache cache, char* url){
     if(url == NULL ||  cache == NULL){
        assert("The parameters can not be null");}
    
    uint32_t hash_value = jenkinsHashFunction(url);
    uint32_t index = hash_value % CACHE_SIZE;

    CacheBlock cblock = cache->cache[index];
    while(cblock != NULL && strcmp(cblock->url, url) != 0 ){
        cblock = cblock->next_element;
    }
    cblock->last_accessed = time(NULL);
    return cblock->content;
}

void resizeCache(Cache *cache){
    printf("resizing the cache\n");
}



int main(int argc, char** argv){
    Cache cache = createCache();

    insertToCache(cache, "https://www.facebook.com", "hacked", 6);
    insertToCache(cache, "https://www.facebook.com", "What are you saying bro?", 24);
    insertToCache(cache, "https://www.facebok.com", "BU ho hunai parcha", 15);
    insertToCache(cache, "https://www.facebok.com", "Repeat", 8);
    insertToCache(cache, "https://www.google.com", "Google", 8);
    char* content = getFromCache(cache, "https://www.facebook.com");
    char* content2 = getFromCache(cache, "https://www.google.com");
    char* content3 = getFromCache(cache, "https://www.facebok.com");
    fprintf(stderr, "\n\nReturning the elements from the cache!\n");

    fprintf(stderr, "%s\n..........\n", content);
    fprintf(stderr, "%s\n..........\n", content2);
    fprintf(stderr, "%s\n..........\n", content3);
    return 0;
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