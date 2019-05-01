/*
* Manish Aryal
* Proxy Cache with HashTable
*/
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "cache.h"

#define CACHE_SIZE 67
#define MAX_ELEMENT 50
#define URL_SIZE 200
#define FILTER_SIZE 256 //4 64 bit uints
#define MAX_AGE 3600
#define OBJECTNAMESIZE 100

//cacheBlock in cache
typedef struct CacheBlock{
    char* url;
    char* content;
    int content_length;
    time_t first_added;
    int max_age;
    struct CacheBlock* next_ll; //next element in the hashtable as a result of chaining
} *CacheBlock;

//cache memory
struct T {
    CacheBlock cache[CACHE_SIZE];
    int total_elements;
};

typedef struct MaxAgeTime {
    time_t last_accessed;
    int max_age;
} MaxAgeTime;

//bloom filters
uint64_t add_filter[4];
uint64_t search_filter[4];

//least recently used removal
MaxAgeTime cache_times[CACHE_SIZE];

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
uint32_t  hash_function1(char *key);
uint32_t  hash_function2( const char *s );

int string_to_int_num2(char *content);
int get_field_value2(char *HTTP_request, char *field, char *storage);
int getMaxAge(char* content, int size);


Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    for(int i = 0; i < CACHE_SIZE; i++){ //initializing the cache elements
        new_cache->cache[i] = NULL;
        cache_times[i].last_accessed = -1;
        cache_times[i].max_age = -1;
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
    int total_elements = cache->total_elements;
    if(total_elements/ CACHE_SIZE > 0.75){
        removeLastAccessed(cache);
    }

    if (content_length == -107 && content == NULL){
        if (url == NULL){
            printf("FOUND THE BBUGGG!!!\n");
        }
        updateSearchBloom(url);
    } else{
        if(isUrlSearched(url)){
            printf("ADDING THE URL TO THE CACHE!!!!\n");
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
                int max_age = getMaxAge(content, content_length);
                new_block->max_age = max_age;
                memcpy(new_block->content, content, content_length);
                memcpy(new_block->url, url, URL_SIZE);
                if(pblock == NULL){ //it is the first element in the index
                    //printf("Inserted at index %d\n", idx);
                    cache->cache[idx] = new_block;
                    cache_times[idx].last_accessed = time(NULL);
                    cache_times[idx].max_age = max_age;
                } else { // it is not the first element in the index
                    //printf("Added at index %d\n", idx);
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

void removeLastAccessed(Cache cache){
    time_t current_time = time(NULL);
    time_t remove_last = time(NULL);
    int index_remove  = -1;
    bool removed = false;
    for(int i = 0; i < CACHE_SIZE; i++){
        time_t last_accessed = cache_times[i].last_accessed;
        int max_age = cache_times[i].max_age;
        if(max_age != -1 && last_accessed != -1){
            if (remove_last >= last_accessed){
                remove_last = last_accessed;
                index_remove = i;
            }
            if(current_time - last_accessed > max_age){
                CacheBlock cblock = cache->cache[i];
                if(cblock != NULL){
                    removed = true;
                    cache->cache[i] = cblock->next_ll;
                    cblock->next_ll = NULL;
                    free(cblock);
                    cache_times[i].max_age = cache->cache[i]->max_age;
                }
            }
        }
    }

    //if nothing removed, remove last_access
    if(!removed && index_remove != -1){
        CacheBlock cblock = cache->cache[index_remove];
        //printf("Cache Block at index %d is removed\n", index_remove);
        if(cblock != NULL){
            cache->cache[index_remove] = cblock->next_ll;
            cblock->next_ll = NULL;
            free(cblock);
            if(cache->cache[index_remove] != NULL){
                cache_times[index_remove].max_age = cache->cache[index_remove]->max_age;
            } else {
                cache_times[index_remove].max_age = -1;
            }
            
        }
    }
}

char* getFromCache(Cache cache, char* url){ //update the linked list for the indices
    if(isUrlPresent(url)){
        printf("URL IS PRESENT IN THE CACHE\n");
        int index = hash_function1(url) % CACHE_SIZE;
        CacheBlock block = cache->cache[index];
        while(block != NULL && strcmp(block->url, url) != 0){
            block = block->next_ll;
        }
        if( block != NULL){ //found the content
            printf("URL IS PRESENT IN THE CACHE\n");
            //printf("Got from cache: %s\n", url);
            cache_times[index].last_accessed = time(NULL);
            cache_times[index].max_age = block->max_age;
            return block->content;
        }
    }
    return NULL;
}


/****************************** BLOOM FILTER FUNCTION *****************************************/
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

int getMaxAge(char* content, int size){
    int max_age = MAX_AGE;
    char* trimmed_object = malloc(size * sizeof(char));
    memcpy(trimmed_object, content, size);

    char *max_age_field_value = calloc(OBJECTNAMESIZE, sizeof(char));
    int status = get_field_value2(trimmed_object, "Cache-Control: max-ag", 
                                max_age_field_value);

    if (status != -1) {
        max_age = string_to_int_num2(max_age_field_value);
    }
    //free(max_age_field_value);
    return max_age;
}



int get_field_value2(char *HTTP_request, char *field, char *storage)
{
    bool field_not_found = true;
    int request_size = strlen(HTTP_request);
    bool match = true;

    for (int i = 0; i < request_size; i++) {
        if (HTTP_request[i] == '\n') {
            match = true;
            for (int k = 0; k < (int)strlen(field); k++) {
                if (HTTP_request[i + k + 1] != field[k]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                i = i + strlen(field) + 3;
                int j = 0;
                while (HTTP_request[i] != '\n' && HTTP_request[i] != '\r'){
                    storage[j] = HTTP_request[i];
                    i++;
                    j++;
                }

                storage[j] = '\0';
                field_not_found = false;
                break;
            }
        }
    }
    if (field_not_found) return -1;
    return 0;
}

int power2(unsigned int a, unsigned int b)
{
    unsigned int answer = 1;
    if (b == 0) return answer;

    answer = a;
    for (unsigned int i = 1; i < b; i++) {
        answer *= a;
    }

    return answer;
}

int string_to_int_num2(char *content)
{
    int j = 0;
    int current_digit;
    int content_length = 0;

    int field_length = strlen(content);
    while (j < field_length) {
        current_digit = content[j] - '0';
        content_length+= current_digit * power2(10, field_length - j - 1);
        j++;
    }

    return content_length;
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
