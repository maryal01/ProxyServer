#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CACHE_SIZE 67
#define MAX_ELEMENT 50
#define URL_SIZE 200
#define FILTER_SIZE 256 //4 64 bit uints
#define MAX_AGE 3600

uint64_t bloom_filter[4];
void setBloomIndex(int hash);
int getBloomIndex(int hash);
uint32_t  hash_function3( const char *s ); 
uint32_t  hash_function2( const char *s );
uint32_t hash_function1(char *key);

int main(){
    char* url = "manish aryal";
    int hash1 = hash_function1(url) % FILTER_SIZE;
    int hash2 = hash_function2(url) % FILTER_SIZE;
    int hash3 = hash_function3(url) % FILTER_SIZE;
    int flag = getBloomIndex(hash1) & getBloomIndex(hash2) & getBloomIndex(hash3);
    setBloomIndex(hash1);
    setBloomIndex(hash2);
    flag = getBloomIndex(hash1) & getBloomIndex(hash2) & getBloomIndex(hash3);
    setBloomIndex(hash3);
    flag = getBloomIndex(hash1) & getBloomIndex(hash2) & getBloomIndex(hash3);
    return flag;
}


void setBloomIndex(int hash){ //call during insertion
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
    flag = flag << index;
    bloom_filter[idx] = bloom_filter[idx] | flag;
}

int getBloomIndex(int hash){ //call during retrieval
    int index = -1;
    uint64_t filter = 0;
    if ( hash < 64){
        index = hash;
        filter = bloom_filter[0];
    } else if( hash < 128){
        index = hash - 64;
        filter = bloom_filter[1];
    } else if ( hash < 192){
        index = hash - 128;
        filter = bloom_filter[2];
    } else if ( hash < 256){
        index = hash - 192;
        filter = bloom_filter[3];
    } else {
        printf("Invalid hash value");
        return -1;
    }
    return (filter << (63 - index)) >> 63 ;
}


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