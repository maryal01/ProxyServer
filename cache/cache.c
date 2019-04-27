#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define CACHE_SIZE 50
#define URL_SIZE 100


typedef struct CacheBlock{
    char* url;
    char* content;
    int content_length;
    time_t last_accessed;
    struct CacheBlock* next_element; //next element in the hashtable as a result of chaining
    struct CacheBlock* previous; //previous element in the remove list
    struct CacheBlock* next; //next element in the remove list
} *CacheBlock;

typedef struct Cache {
    CacheBlock cache[CACHE_SIZE];
    int total_elements;
} *Cache;


CacheBlock head; //needed to remove the element in O(1) time
CacheBlock tail; //needed to update the position of the element, already in the list, in O(1) time

uint32_t jenkinsHashFunction(char *key);
CacheBlock returnCacheBlock(CacheBlock cache, char* url);
char* getFromCache(Cache cache, char* url);
void insertToCache(Cache cache, char* url, char* content, int content_length);
CacheBlock returnCacheBlock(CacheBlock cblock, char* url);
void removeLastAccessed(Cache cache);
void resizeCache(Cache *cache);


Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    head = NULL;
    tail = NULL;
    for(int i = 0; i < CACHE_SIZE; i++){
        new_cache->cache[i] = NULL;
    }
    return new_cache;

}

CacheBlock createCacheBlock(){
    CacheBlock block = malloc(sizeof(*block));
    block->last_accessed = time(NULL);
    block->next_element = NULL;
    block->previous = tail;
    block->next = NULL;
    tail = block;
    if(head == NULL) { head = block; }
    return block;
}

void insertToCache(Cache cache, char* url, char* content, int content_length){

    if(url == NULL || content == NULL || cache == NULL){
        assert("The parameters can not be null");}
   
    CacheBlock block = createCacheBlock();
    block->url = malloc(URL_SIZE);
    block->content = malloc(content_length);
    block->content_length = content_length;
    memcpy(block->url, url, URL_SIZE);
    memcpy(block->content, content, content_length);
    

    uint32_t hash_value = jenkinsHashFunction(url);
    uint32_t index = hash_value % CACHE_SIZE;

    CacheBlock current_block = cache->cache[index];
    if(current_block == NULL){ //if nothing is inserted at that index in hashtable
        cache->cache[index] = block;
    } else {
        CacheBlock cblock = returnCacheBlock(current_block, url);
        if( strcmp(cblock->url, url) == 0){ // found the element traversing through chained linked list
            free(block);
            int total_size = content_length + cblock->content_length;
            char* total_content = malloc(total_size);
            memcpy(total_content, cblock->content, cblock->content_length);
            memcpy(total_content + cblock->content_length, content, content_length);
            free(cblock->content);

            cblock->content = malloc(total_size);
            cblock->content_length = total_size;
            memcpy(cblock->content, total_content, total_size);
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
    int index_to_remove = -1;
    time_t earliest_time = time(NULL);
    for(int i = 0; i < CACHE_SIZE; i++){
        CacheBlock cblock = cache->cache[i];
        if(cblock != NULL){
            if (earliest_time > cblock->last_accessed){
                earliest_time = cblock->last_accessed;
                index_to_remove = i;
            }
        }
    }

    CacheBlock remove_cache = cache->cache[index_to_remove];
    CacheBlock replace_cache = remove_cache->next_element;
    cache->cache[index_to_remove] = replace_cache;
    remove_cache->next_element = NULL;
    free(remove_cache);
}

void updateRemovalList(CacheBlock cblock){
    if(cblock->next != NULL){
        cblock->next->previous = cblock->previous;
        if(cblock = head){ head = cblock->next; }
        cblock->next = NULL;
    }
    if(cblock->previous != NULL){
        cblock->previous->next = cblock->next;
        cblock->previous = NULL;
    }
    tail->next = cblock;
    cblock->previous = tail;
    tail = cblock;
}

char* getFromCache(Cache cache, char* url){
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
        updateRemovalList();
        cblock->last_accessed = time(NULL);
        return cblock->content;
    }
    
    return NULL;
}

void resizeCache(Cache *cache){
    printf("resizing the cache\n");
}


uint32_t  ElfHash ( const char *s );
int main(int argc, char** argv){
    uint32_t hash_value1 = ElfHash("https://www.google.com");
    uint32_t hash_value2 = ElfHash("https://www.google.com");
    uint32_t hash_value3 = ElfHash("https://www.facebook.com");
    fprintf(stderr, "values: %u, %u, %u", hash_value1, hash_value2, hash_value3);
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


uint32_t  ElfHash ( const char *s )
{
    uint32_t  h = 0, high;
    while ( *s )
    {
        h = ( h << 4 ) + *s++;
        if ( high == (h & 0xF0000000 ))
            h ^= high >> 24;
        h &= ~high;
    }
    return h;
}

