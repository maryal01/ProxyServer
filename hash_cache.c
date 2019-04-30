/*
* Manish Aryal
* Proxy Cache with HashTable
*/
#include <time.h>
#include <string.h>
#include <assert.h>
#include "hash_functions.h"
#include "cache.h"

#define CACHE_SIZE 67
#define MAX_ELEM_SIZE 50
#define URL_SIZE 100
#define FILTER_SIZE 256 //4 64 bit uints
#define MAX_AGE 3600

//cacheBlock in cache
typedef struct CacheBlock{
    char* url;
    char* content;
    int content_length;
    time_t last_accessed;
    struct CacheBlock* next_ll; //next element in the hashtable as a result of chaining
} *CacheBlock;

//cache memory
struct T {
    CacheBlock cache[CACHE_SIZE];
    int total_elements;
};

//bloom filters
uint64_t add_bloom_filter = 0;
uint64_t search_bloom_filter[4];

//least recently used removal
int hash_indices[CACHE_SIZE];
int head_index;
int tail_index;

//intermediate value
CacheBlock intermediate_value;

void setBloomIndex(uint64_t bloom_filter, int i){
    return bloom_filter & (1 << i);
}

int getBloomIndex(uint64_t bloom_filter, int i){
    return bloom_filter >> (63 - i) << 63;
}

Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    intermediate_value = NULL;
    head_index = -1;
    tail_index = 0;
    for(int i = 0; i < CACHE_SIZE; i++){
        new_cache->cache[i] = NULL;
        hash_indices[i] = -1;
    }
    return new_cache;
}

CacheBlock createCacheBlock(){
    CacheBlock block = malloc(sizeof(*block));
    block->last_accessed = time(NULL);
    block->next_ll = NULL;
}

int searchedAlready(char* url){
    int hash1 = hash_function1(url) % CACHE_SIZE;
    int hash2 = hash_function2(url) % CACHE_SIZE;
    int flag = getBloomIndex(add_bloom_filter, hash1) & getBloomIndex(add_bloom_filter, hash2);

    setBloomIndex(add_bloom_filter, hash1);
    setBloomIndex(add_bloom_filter, hash2);

    return flag;
}

void insertToCache(Cache cache, char* url, char* content, int content_length){
    /*
        if intermediate value is null
            store it there
        if intermediate value's url match
            append it there
        if intermediate value's url don't match
            check the bloom filters
            if set
                store it to the cache
            if not set
                don't store it to cache, but set the flag
    */
    
    int flag = searchedAlready(url);
    if (flag){
        uint32_t hash_value = hash_function1(url);
        uint32_t index = hash_value % CACHE_SIZE;
        CacheBlock current_block = cache->cache[index];

        if(current_block == NULL){
            CacheBlock block = createCacheBlock();
            block->url = malloc(URL_SIZE);
            block->content = malloc(content_length);
            block->content_length = content_length;
            memcpy(block->url, url, URL_SIZE);
            memcpy(block->content, content, content_length);
            cache->cache[index] = block;
        
        } else {
            CacheBlock cblock = returnCacheBlock(current_block, url);
            if( strcmp(cblock->url, url) == 0){ 
                appendContent(cblock, content, content_length);  
                return;
            }
            CacheBlock block = createCacheBlock();
            block->url = malloc(URL_SIZE);
            block->content = malloc(content_length);
            block->content_length = content_length;
            memcpy(block->url, url, URL_SIZE);
            memcpy(block->content, content, content_length);
            cblock->next_ll = block;
            
        }
    }
}

void appendContent(CacheBlock cblock, char* content, int content_len){
    int total_size = cblock->content_length + content_len;
    char* total_content = malloc(total_size);
    memcpy(total_content, cblock->content, cblock->content_length);
    memcpy(total_content + cblock->content_length, content, content_len);
    free(cblock->content);
    cblock->content = malloc(total_size);
    cblock->content_length = total_size;
    memcpy(cblock->content, total_content, total_size);
}

CacheBlock returnCacheBlock(CacheBlock cblock, char* url){
    CacheBlock pblock = NULL;
    while(cblock != NULL && strcmp(cblock->url, url) != 0){
        pblock = cblock;
        cblock = cblock->next_ll;
    }
    if(cblock == NULL){
        return pblock;
    }
    return cblock;
}


void removeLastAccessed(Cache cache){

}


char* getFromCache(Cache cache, char* url){
     if(url == NULL ||  cache == NULL){
        assert("The parameters can not be null");}
    
    uint32_t hash_value = hash_function1(url);
    uint32_t index = hash_value % CACHE_SIZE;
    
    CacheBlock cblock = cache->cache[index];
    while(cblock != NULL && strcmp(cblock->url, url) != 0 ){
        cblock = cblock->next_ll;
    }
    if (cblock != NULL){
        updateRemovalList();
        cblock->last_accessed = time(NULL);
        return cblock->content;
    }
    
    return NULL;
}

int content_size(Cache cache, char* url)
{
    fprintf(stderr, "The url passed here is: %s\n", url);
     if(url == NULL ||  cache == NULL){
        assert("The parameters can not be null");}
    
    uint32_t hash_value = jenkinsHashFunction(url);
    uint32_t index = hash_value % CACHE_SIZE;
    
    CacheBlock cblock = cache->cache[index];
    while(cblock != NULL && strcmp(cblock->url, url) != 0 ){
        cblock = cblock->next_element;
    }
    if (cblock != NULL){
        return cblock->content_length;
    }
    
    return -1;
}

int getMaxCache(char* content){
    return MAX_AGE;
}