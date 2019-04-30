/*
* Manish Aryal
* Proxy Cache with HashTable
*/
#include <time.h>
#include <string.h>
#include <assert.h>
#include "cache.h"

#define CACHE_SIZE 67
#define MAX_ELEMENT 50
#define URL_SIZE 200
#define FILTER_SIZE 256 //4 64 bit uints
#define MAX_AGE 3600

//cacheBlock in cache
typedef struct CacheBlock{
    char* url;
    char* content;
    int content_length;
    time_t first_added;
    struct CacheBlock* next_ll; //next element in the hashtable as a result of chaining
} *CacheBlock;

//cache memory
struct T {
    CacheBlock cache[CACHE_SIZE];
    int total_elements;
};

//bloom filters
uint64_t add_filter[4];
uint64_t search_filter[4];

//least recently used removal
int hash_indices[CACHE_SIZE];
int head_index;
int tail_index;

void updateAddedBloom(char* url);
int getMaxCache(char* content);
int getAddedBloom(int hash);
void setAddedBloom(int hash);
int isUrlPresent(char* url);

void updateSearchBloom(char* url);
int getSearchBloom(int hash);
void setSearchBloom(int hash);
int isUrlSearched(char* url);

uint32_t  hash_function3( const char *s );
uint32_t hash_function1(char *key);
uint32_t  hash_function2( const char *s );

Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    head_index = 0;
    tail_index = 0;
    for(int i = 0; i < CACHE_SIZE; i++){ //initializing the cache elements
        new_cache->cache[i] = NULL;
        hash_indices[i] = -1;
    }
    for(int i = 0; i < 4; i++ ){
        add_filter[i] = 0;
        search_filter[i] = 0;
    }
    new_cache->total_elements = 0;
    return new_cache;
}

CacheBlock createCacheBlock(){
    CacheBlock block = malloc( sizeof( *block ) );
    block->first_added = time(NULL);
    block->next_ll = NULL;
    return block;
}


void insertToCache(Cache cache, char* url, char* content, int content_length){

    if (content_length == -107 && content == NULL){
        updateSearchBloom(url);
    } else{
        if(isUrlSearched(url)){
            //add to the cache
            int idx = hash_function1(url) % CACHE_SIZE;
            CacheBlock block = cache->cache[idx];
            CacheBlock pblock = block;
            while( block != NULL && strcmp(block->url, url) != 0){ // TODO: remove the while loop with tail
                pblock = block;
                block = block->next_ll;
            }
            if(block == NULL){ //adding a new element
                cache->total_elements += 1;
                CacheBlock new_block = createCacheBlock();
                new_block->content = malloc(content_length);
                new_block->url     = malloc(URL_SIZE);
                new_block->content_length = content_length;
                memcpy(new_block->content, content, content_length);
                memcpy(new_block->url, url, URL_SIZE);
                if(pblock == NULL){ //it is the first element in the index
                    cache->cache[idx] = new_block;
                } else { // it is not the first element in the index
                    pblock->next_ll = new_block;
                }
                updateAddedBloom(url);
            } else { // appending already present element
                int content1 = block->content_length;
                char* total_content = malloc(content_length + content1);
                memcpy(total_content, block->content, content1);
                memcpy(total_content + content1, content, content_length);
                free(block->content);
                block->content = malloc(content1 + content_length);
                block->content_length = content1 + content_length;
                memcpy(block->content, total_content, content1 + content_length);
            }
        }
    }
    
}

void updateAddedBloom(char* url){
    int hash1 = hash_function1(url) % FILTER_SIZE;
    int hash2 = hash_function2(url) % FILTER_SIZE;
    int hash3 = hash_function3(url) % FILTER_SIZE;
    setAddedBloom(hash1);
    setAddedBloom(hash2);
    setAddedBloom(hash3);
}

void updateSearchBloom(char* url){
    int hash1 = hash_function1(url) % FILTER_SIZE;
    int hash2 = hash_function2(url) % FILTER_SIZE;
    int hash3 = hash_function3(url) % FILTER_SIZE;
    setSearchBloom(hash1);
    setSearchBloom(hash2);
    setSearchBloom(hash3);
}

void removeLastAccessed(){ //TODO: implement this with the int hash table
    return;
}

char* getFromCache(Cache cache, char* url){
    if(isUrlPresent(url)){
        int index = hash_function1(url) % CACHE_SIZE;
        CacheBlock block = cache->cache[index];
        while(block != NULL && strcmp(block->url, url) != 0){
            block = block->next_ll;
        }
        if( block != NULL){ //found the content
            return block->content;
        }
    }
    return NULL;
}

int isUrlPresent(char* url){
    int hash1 = hash_function1(url) % FILTER_SIZE;
    int hash2 = hash_function2(url) % FILTER_SIZE;
    int hash3 = hash_function3(url) % FILTER_SIZE;
    int flag = getAddedBloom(hash1) & getAddedBloom(hash2) & getAddedBloom(hash3);
    return flag;
}

int isUrlSearched(char * url){
    int hash1 = hash_function1(url) % FILTER_SIZE;
    int hash2 = hash_function2(url) % FILTER_SIZE;
    int hash3 = hash_function3(url) % FILTER_SIZE;
    int flag = getSearchBloom(hash1) & getSearchBloom(hash2) & getSearchBloom(hash3);
    return flag;
}

void setSearchBloom(int hash){
    int index = -1;
    int idx = -1;
    if ( hash < 64){
        index = hash;
        idx = 0;
    } else if( hash < 128){
        index = hash - 64;
        idx = 1;
    } else if ( hash < 192){
        index = hash - 128;
        idx = 2;
    } else if ( hash < 256){
        index = hash - 192;
        idx = 3;
    } else {
        printf("Invalid hash value");
        return;
    }
    uint64_t flag = 1;
    flag= flag << index;
    search_filter[idx] = search_filter[idx] | flag;
}

int getSearchBloom(int hash){
    int index = -1;
    uint64_t filter = 0;
    if ( hash < 64){
        index = hash;
        filter = search_filter[0];
    } else if( hash < 128){
        index = hash - 64;
        filter = search_filter[1];
    } else if ( hash < 192){
        index = hash - 128;
        filter = search_filter[2];
    } else if ( hash < 256){
        index = hash - 192;
        filter = search_filter[3];
    } else {
        printf("Invalid hash value");
        return -1;
    }

   return (filter << (63 - index)) >> 63 ;
}


void setAddedBloom(int hash){ //call during insertion
    int index = -1;
    int idx = -1;
    if ( hash < 64){
        index = hash;
        idx = 0;
    } else if( hash < 128){
        index = hash - 64;
        idx = 1;
    } else if ( hash < 192){
        index = hash - 128;
        idx = 2;
    } else if ( hash < 256){
        index = hash - 192;
        idx = 3;
    } else {
        printf("Invalid hash value");
        return;
    }
    uint64_t flag = 1;
    flag= flag << index;
    add_filter[idx] = add_filter[idx] | flag;
}

int getAddedBloom(int hash){ //call during retrieval
    int index = -1;
    uint64_t filter = 0;
    if ( hash < 64){
        index = hash;
        filter = add_filter[0];
    } else if( hash < 128){
        index = hash - 64;
        filter = add_filter[1];
    } else if ( hash < 192){
        index = hash - 128;
        filter = add_filter[2];
    } else if ( hash < 256){
        index = hash - 192;
        filter = add_filter[3];
    } else {
        printf("Invalid hash value");
        return -1;
    }

   return (filter << (63 - index)) >> 63 ;
}



/****************************** UTILITY FUNCTION *****************************************/
int content_size(Cache cache, char* url)
{
    fprintf(stderr, "The url passed here is: %s\n", url);
     if(url == NULL ||  cache == NULL){
        assert("The parameters can not be null");}
    
    uint32_t hash_value = hash_function1(url);
    uint32_t index = hash_value % CACHE_SIZE;
    
    CacheBlock cblock = cache->cache[index];
    while(cblock != NULL && strcmp(cblock->url, url) != 0 ){
        cblock = cblock->next_ll;
    }
    if (cblock != NULL){
        return cblock->content_length;
    }
    
    return -1;
}

int getMaxCache(char* content){
    (void) content;
    return MAX_AGE;
}

/****************************** HASH FUNCTION *****************************************/
uint32_t hash_function1(char *key)
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


uint32_t  hash_function2( const char *s )
{
    uint32_t  h = 0, high = 0;
    while ( *s )
    {
        h = ( h << 4 ) + *s++;
        if ( high == (h & 0xF0000000 ))
            h ^= high >> 24;
        h &= ~high;
    }
    return h;
}

uint32_t  hash_function3( const char *s ) //change this hash function
{
    return strlen(s) + 13;
}
