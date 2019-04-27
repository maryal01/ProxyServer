#include "cache.h"
#include <string.h>

int main(int argc, char** argv){
    (void) argc;
    (void) argv;
    Cache cache = createCache();
    insertToCache(cache, "file://a.html", "1234567890", 10);
    insertToCache(cache, "file://b.html", "qwertyuiop", 10);
    char* content1 = getFromCache(cache, "file://a.html");
    char* content2 = getFromCache(cache, "file://b.html");

    fprintf(stderr,"Content1: %s \n and Content2: %s \n", content1, content2);
    return 0;
}
