/*
* Manish Aryal
* Hash Functions
*/

#ifndef HASH_FUNCTIONS
#define HASH_FUNCTIONS

#include <stdlib.h>
#include <stdint.h>


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

#endif