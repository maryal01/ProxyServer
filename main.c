#include "cache.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define URL_SIZE 200
void assert(char* got, char* expected, char* message){
    int value = 0;
    if (got == NULL  || expected == NULL){
        value = got == expected;
    } else {
        value = strcmp(got, expected) == 0;
    }
    if ( value == 0 ){
        printf("Test Failed: %s", message);
        //printf("Got: %s Expected: %s\n", got, expected);

    } else {
        printf("Test Passed: %s", message);
    }
    printf("------------------------------\n\n");
}

void test0(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(content, NULL, "First time access: no content\n");

}

void test1(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(content, "gfhjioyutdfghv", "Second time access: content\n");
}

void test2(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioy", 7);
    insertToCache(cache, "url", "utd", 3);
    insertToCache(cache, "url", "fghv", 4);
    insertToCache(cache, "url", NULL, -107);
    char* content = getFromCache(cache, "url");
    assert(content, NULL, "First time access: Append functionality\n");
}

void test3(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "ursfal", "gfhjioyutdfghv", 14);
    insertToCache(cache, "ursfal", NULL, -107);
    insertToCache(cache, "uasfal", "gfhjioyutdfghv", 14);
    insertToCache(cache, "uasfal", NULL, -107);
    insertToCache(cache, "uasfal", "gfhjioyutdfghv", 14);
    char* content1 = getFromCache(cache, "url");
    char* content2 = getFromCache(cache, "uasfal");
    char* content3 = getFromCache(cache, "ursfal");
    assert(content1, content2, "Second time access: Multiple insertion test\n");
    assert(content3,  NULL, "First time access: Multiple insertion test\n");
}

void test4(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioy", 14);
    insertToCache(cache, "url", "utd", 13);
    insertToCache(cache, "url", "fghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(content, "gfhjioyutdfghv", "Invalid size: more than the content\n");
}

void test5(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioy", 7);
    insertToCache(cache, "url", "utd", 3);
    insertToCache(cache, "url", "fghv", 4);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    assert(content, "gfhjioyutdfghv", "Second time access: Append functionality\n");
}

void test6(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    char* content = getFromCache(cache, "url");
    sleep(2);
    assert(content, "gfhjioyutdfghv", "Basic remove functionality test\n");
    removeLastAccessed(cache);
    sleep(2);
    content = getFromCache(cache, "url");
    assert( content, NULL, "Basic remove functionality test\n");
}

void test7(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url2", NULL, -107);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url2");
    sleep(2);
    content = getFromCache(cache, "url");
    removeLastAccessed(cache);
    content = getFromCache(cache, "url2");
    assert( content, NULL, "Logic 1 for remove functionality\n");
}

void test8(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url2", NULL, -107);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url");
    sleep(2);
    content = getFromCache(cache, "url2");
    removeLastAccessed(cache);
    content = getFromCache(cache, "url");
    assert( content, NULL, "Logic 2 for remove functionality\n");
}

void test9(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url2", NULL, -107);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url");
    sleep(2);
    content = getFromCache(cache, "url2");
    removeLastAccessed(cache);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    sleep(2);
    content = getFromCache(cache, "url");

    assert( content, "gfhjioyutdfghv", "Inserting once after removing should still keep it on the table\n");
}

void test10(){
    Cache cache = createCache();
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url2", "gfhjioyutdfghv", 14);
    
    char* content = getFromCache(cache, "url");
    sleep(2);
    content = getFromCache(cache, "url2");
    removeLastAccessed(cache);
    
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);
    insertToCache(cache, "url", NULL, -107);
    insertToCache(cache, "url", "gfhjioyutdfghv", 14);

    content = getFromCache(cache, "url");
    assert( content, "gfhjioyutdfghvgfhjioyutdfghv", "Inserting twice after removing\n");
}

int main(int argc, char** argv){
    (void) argc;
    (void) argv;
    test0();
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
}
