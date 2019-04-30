#include "cache.h"
#include <string.h>
void assert(int value, char* message){
    if ( value == 0 ){
        printf("Test Failed: %s", message);
    } else {
        printf("Test Passed: %s", message);
    }
}

void test0(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(content == NULL, "First time access: no content");

}

void test1(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(strcmp(content, "gfhjioyutdfghv") == 0, "Second time access: content");
}

void test2(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioy", 7);
    insertToCache(cache, "url", "utd", 3);
    insertToCache(cache, "url", "fghv", 4);
    char* content = getFromCache(cache, "url");
    assert(content == NULL, "First time access: Append functionality");
}

void test3(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "ursfal", "gfhjioyutdfghv", 14);
    insertToCache(cache, "uasfal", "gfhjioyutdfghv", 14);
    insertToCache(cache, "uasfal", "gfhjioyutdfghv", 14);
    char* content1 = getFromCache(cache, "url");
    char* content2 = getFromCache(cache, "uasfal");
    char* content3 = getFromCache(cache, "ursfal");
    assert(strcmp(content1, content2) == 0, "Second time access: Multiple insertion test");
    assert(content3 == NULL, "First time access: Multiple insertion test");
}

void test4(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioy", 14);
    insertToCache(cache, "url", "utd", 13);
    insertToCache(cache, "url", "fghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(strcmp(content, "gfhjioyutdfghv") == 0, "Invalid size: more than the content");
}

void test5(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioy", 7);
    insertToCache(cache, "url", "utd", 3);
    insertToCache(cache, "url", "fghv", 4);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(content == NULL, "Second time access: Append functionality");
}

void test6(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(strcmp(content, "gfhjioyutdfghv") == 0, "Basic remove functionality tests");
    removeLastAccessed(cache);
    content = getFromCache(cache, "url");
    assert( content == NULL, "Basic remove functionality test");
}

void test7(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url2");
    content = getFromCache(cache, "url");
    removeLastAccessed(cache);
    content = getFromCache(cache, "url2");
    assert( content == NULL, "Logic 1 for remove functionality");
}

void test8(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url");
    content = getFromCache(cache, "url2");
    removeLastAccessed(cache);
    content = getFromCache(cache, "url");
    assert( content == NULL, "Logic 2 for remove functionality");
}

void test9(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url");

    content = getFromCache(cache, "url2");
    removeLastAccessed(cache);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    content = getFromCache(cache, "url");

    assert( content != NULL, "Inserting once after removing should still keep it on the table ");
}

void test10(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url");

    content = getFromCache(cache, "url2");
    removeLastAccessed(cache);
    
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);

    content = getFromCache(cache, "url");
    assert( strcmp(content, "gfhjioyutdfghv") == 0, "Inserting twice after removing ");
}

int main(int argc, char** argv){
    (void) argc;
    (void) argv;

}