/*
    TODO: 
    GET request's header lists url as: http://www.google.com/
    while
    CONNECT requests's header lists url as: www.google.com:443
    so
    when parsing the header for the resource url, different strings will
    be generated. This can be a problem when looking up the cache. Imagine:
    someone visits http://www.google.com and then www.google.com:443. Cache
    won't come into play, even though it theoretically should.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <bandwidth.h>

/* pending connection queue's max length */
#define BACKLOG 20
#define BUFSIZE 1000000
/* max response (webpage) size */
#define OBJECTSIZE 10000000
/* size of cache */
#define CACHELINES 20
/* client request's single line max size */
#define OBJECTNAMESIZE 100
#define DEFAULTPORT 80
#define HEADERSIZE 64000
#define SIZEPADDING 10
#define CONCURRENTCONNECTIONS 20
#define GET 0
#define CONNECT 1

/* Manish's structs */
#define CONTENT_SIZE 100
#define CACHE_SIZE 67
#define MAX_ELEMENT 50
#define URL_SIZE 200
#define FILTER_SIZE 256 //4 64 bit uints
#define MAX_AGE 3600

//bloom filters
uint64_t bloom_filter[4];

//least recently used removal
int hash_indices[CACHE_SIZE];
int head_index;
int tail_index;

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

/* Manish's code */




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




/* full_data is the full request message from client */
typedef struct client_request {
        char hostname[OBJECTNAMESIZE]; 
        int port;
        char *full_data;
} *client_request;

/* client-server connection pairs */
/* method can be GET or CONNECT  */
typedef struct connection {
        int method;
        char *url;
        int clientfd;
        int serverfd; 
} *connection;

/* single cache line (will be removed once Manish implements his cache with
cacheblocks) */
typedef struct cache_line {
        char *object;
        /* etc ... */
} *cache_line;

fd_set active_fd_set, read_fd_set;

void error(char *msg);
void ensure_argument_validity(int argc, char *executable_name);
int create_socket(uint16_t port);
void setup_cache(cache_line *cache);
void setup_connections(connection *connections);
bool connection_exists(int fd, connection *connections);
int build_client_request(char *full_data, client_request *request);
int set_portno(char *full_data, int *port);
int set_hostname(char *full_data, char *hostname);
int get_field_value(char *HTTP_request, char *field, char *storage);
int string_to_int_num(char *content);
int power(unsigned int a, unsigned int b);
bool CONNECT_request(char *full_data);
int establish_connection_with_server(int *serverfd, client_request request);
int partner(int fd, connection *connections);
void create_connection_pair(int clientfd, int serverfd, int method, char* url, connection *connections);
void remove_connection_pair(int fd, connection *connections);
int total_response_size(char *object);
void send_HTTP_OK(int clientfd);
char *get_url(char *HTTP_request);
/* returns 0 for GET, and 1 for CONNECT */
int connection_pair_method(int fd, connection *connections);
char *connection_pair_url(int fd, connection *connections);
int content_size(Cache cache, char* url);


/* Manish's functions */
Cache createCache();
CacheBlock returnCacheBlock(CacheBlock cache, char* url);
uint32_t jenkinsHashFunction(char *key);
char* getFromCache(Cache cache, char* url);
void insertToCache(Cache cache, char* url, char* content, int content_length);
/* Manish's functions */

/* Roy's function */ 
bool is_client(int fd, connection *connections);
/* Roy's function */ 

int main(int argc, char *argv[])
{
    int i, m;
    int partnerfd;
    int serverfd;
    int server, port;
    int new_client, client_size;
    struct sockaddr_in clientname;
    struct timeval tv;
    char buffer[BUFSIZE];
    char *response;
    int response_size;
    int status_code;
    char *objectFromCache;
    char *url;
    int size;
    cache_line cache[CACHELINES];
    connection connections[CONCURRENTCONNECTIONS];
    client_request request;


    

    ensure_argument_validity(argc, argv[0]);
    port = atoi(argv[1]);
    
    /* BANDWIDTH LIMIT INITIALIZE */
    /* added to get the maximum bandwidth for the bandwidth interface */
    int max_bandwidth = 0;
    if (argc == 3) {
        max_bandwidth = atoi(argv[2]);
        limit_set_bandwidth(max_bandwidth);
    }
    else 
        limit_set_bandwidth(0); // If bandwidth = 0, bandwidth option is off,
    /* BANDWIDTH LIMT INITIALIZE */

    server = create_socket(port);
    if (listen (server, BACKLOG) < 0) 
        error("listen");

    FD_ZERO (&active_fd_set);
    FD_SET (server, &active_fd_set);

    setup_cache(cache);
    setup_connections(connections);
    Cache proxyCache = createCache();

    while (true) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        /* BANDWIDTH LIMIT WRITE */ 
        // for each socket, write to client if fd is in 
        for (int i = 0; i < CONCURRENTCONNECTIONS; ++i) {
            m = limit_write(i);
            if (m < 0) {
                fprintf(stderr, "ERROR writing from limiting bandwidth");
                remove_connection_pair(i, connections);
                close(i);
                partnerfd = partner(i, connections);
                close(partnerfd);
                // BANDWIDTH LIMIT CLEAR fd info ON ERROR
                limit_clear(i);
            }
        }
        /* BANDWIDTH LIMIT WRITE */ 

        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL,  &tv) < 0)
            error("select");

        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == server)
                {
                    fprintf(stderr, "connected\n");
                    client_size = sizeof(clientname);
                    new_client = accept (server,
                                (struct sockaddr *) &clientname,
                                &client_size);
                    if (new_client < 0) 
                        error("ERROR accepting new client");
                    
                    FD_SET (new_client, &active_fd_set);
                }
                else
                {
                    fprintf(stderr, "received some data\n");
                    bzero(buffer, BUFSIZE);
                    // wait so I could send all the message first
                    if (!limit_read_wait(i)) {
                        m = read(i, buffer, BUFSIZE);
                        if (m <= 0) {
                            fprintf(stderr,"read 0 or less bytes\n");
                            if (connection_exists(i, connections)) {
                                partnerfd = partner(i, connections);
                                close(partnerfd);
                                FD_CLR(partnerfd, &active_fd_set);
                                remove_connection_pair(i, connections);
                                /* BANDWIDTH LIMIT CLEAR */
                                limit_clear(partnerfd);
                            }
                            close(i);
                            /* BANDWIDTH LIMIT CLEAR */
                            limit_clear(i);
                            FD_CLR(i, &active_fd_set);
                            continue;
                        }
                    }

                    if (connection_exists(i, connections)) {
                        fprintf(stderr, "connection exists\n");
                        if(connection_pair_method(i, connections) == GET) {
                            fprintf(stderr, "inserting to cache\n");
                            insertToCache(proxyCache, connection_pair_url(i, connections), buffer, m);
                        }
                        partnerfd = partner(i, connections);
                        /* BANDWIDTH LIMIT SAVE FOR UNCACHED */
                        // more explanations in bandwidth.h file
                        if (is_client(partnerfd, connections))
                            limit_save(partnerfd, buffer, m, false);
                        
                        // simply write to server (e.g. facebook.com)
                        // if partnerfd is a server
                        else {
                            m = write(partnerfd, buffer, m);
                            if (m < 0) {
                                remove_connection_pair(i, connections);
                                close(i);
                                partnerfd = partner(i, connections);
                                close(partnerfd);
                                continue;
                            }
                        }
                        /* BANDWIDTH LIMIT SAVE FOR UNCACHED */
                    } else {
                        fprintf(stderr, "%s\n", buffer);
                        fprintf(stderr, "connection doesn't exist\n");
                        status_code = build_client_request(buffer, &request);
                        if (status_code == -1) {
                            if (request)
                                free(request);
                            close(i);
                            FD_CLR(i, &active_fd_set);
                            continue;
                        }
                        url = get_url(buffer);
                        fprintf(stderr, "url=%s\n", url);
                        if (CONNECT_request(buffer)) {
                            fprintf(stderr, "CONNECT request\n");
                            status_code = establish_connection_with_server(&serverfd, request);
                            if (status_code == -1) {
                                free(request);
                                close(i);
                                FD_CLR(i, &active_fd_set);
                                continue;
                            }
                            FD_SET (serverfd, &active_fd_set);
                            create_connection_pair(i, serverfd, CONNECT, url, connections);
                            send_HTTP_OK(i);
                        } else {
                            fprintf(stderr, "GET request\n");
                            objectFromCache = getFromCache(proxyCache, url);
                            size = content_size(proxyCache, url);
                            if (objectFromCache) {
                                fprintf(stderr, "resource found in cache \n");
                                /* BANDWIDTH LIMIT SAVE (commented out original) */
                                limit_save(i, objectFromCache, size, true);
                                /* ORIGINAL
                                m = write(i, objectFromCache, size);
                                if (m < 0) {
                                    free(request);
                                    remove_connection_pair(serverfd, connections);
                                    close(i);
                                    partnerfd = partner(i, connections);
                                    close(partnerfd);
                                    continue;
                                }*/
                            } else {
                                fprintf(stderr, "resource NOT found in cache \n");
                                status_code = establish_connection_with_server(&serverfd, request);
                                if (status_code == -1) {
                                    free(request);
                                    close(i);
                                    FD_CLR(i, &active_fd_set);
                                    continue;
                                }
                                FD_SET (serverfd, &active_fd_set);
                                create_connection_pair(i, serverfd, GET, url, connections);
                                m = write(serverfd, buffer, strlen(buffer));
                                if (m < 0) {
                                    free(request);
                                    remove_connection_pair(serverfd, connections);
                                    close(i);
                                    partnerfd = partner(i, connections);
                                    close(partnerfd);
                                    continue;
                                }
                                fprintf(stderr, "request sent\n");
                            }
                        }
                        if (request)
                                free(request);
                    }
                }
            }
    }

    return EXIT_SUCCESS;
}

void error(char *msg) 
{
    perror(msg);
    exit(1);
}

/* ensures that port number is provided with executable name */
/*********** added argument 3 for bandwidth control option ************/
void ensure_argument_validity(int argc, char *executable_name)
{
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: %s <port> or\n", executable_name);
        // Bandwidth taking argument
        fprintf(stderr, "usage: %s <port> <max_bandwidth>\n", executable_name);
        exit(EXIT_FAILURE);
    }
}

/* creates the proxy server's socket */
int create_socket(uint16_t port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket to clients"); 

    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &server_addr,
              sizeof(server_addr)) < 0) 
              error("ERROR binding");

    return sockfd;
}

/* prepares the cache by setting every line to NULL */
void setup_cache(cache_line *cache)
{
    for (int i = 0; i < CACHELINES; i++) {
        cache[i] = NULL;
    }
}

void setup_connections(connection *connections)
{
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        connections[i] = NULL;
    }
}

int partner(int fd, connection *connections)
{
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] != NULL) {
            if (connections[i]->clientfd == fd)
                return connections[i]->serverfd;
            if (connections[i]->serverfd == fd)
                return connections[i]->clientfd;
        }
    }
}

void remove_connection_pair(int fd, connection *connections)
{
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] != NULL) {
            if (connections[i]->clientfd == fd || connections[i]->serverfd == fd) {
                free(connections[i]->url);
                free(connections[i]);
                connections[i] = NULL;
            }
        }
    }
}

void create_connection_pair(int clientfd, int serverfd, int method, char *url, connection *connections)
{
    connection tmp = malloc(sizeof(*tmp));
    assert(tmp);
    tmp->clientfd = clientfd;
    tmp->serverfd = serverfd;
    tmp->method = method;
    tmp->url = url;

    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] == NULL) {
            connections[i] = tmp;
            return;
        }
    }

}

bool connection_exists(int fd, connection *connections)
{
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] != NULL) {
            if (connections[i]->clientfd == fd || connections[i]->serverfd == fd)
            return true;
        }
    }

    return false;
}

/* if something went wrong returns -1 */
int build_client_request(char *full_data, client_request *request)
{
    int status_code;

    client_request tmp = malloc(sizeof(*tmp));
    tmp->full_data = full_data;
    tmp->port = DEFAULTPORT;
    status_code = set_portno(full_data, &(tmp->port));
    if (status_code == -1) return -1;
    bzero(&(tmp->hostname), OBJECTNAMESIZE);
    status_code = set_hostname(full_data, tmp->hostname);
    if (status_code == -1) return -1;
    *request = tmp;
    return 1;
}

int set_portno(char *full_data, int *port)
{
    char *host_field_value = calloc(OBJECTNAMESIZE, sizeof(char));
    assert(host_field_value);
    int status = get_field_value(full_data, "Host", host_field_value);
    if (status == -1) {
        fprintf(stderr, "Invalid Request\n");
        free(host_field_value);
        return -1;
    }
    int field_length = strlen(host_field_value);
    int current_digit;

    for (int i = 0; i < field_length; i++) {
        if (host_field_value[i] == ':') {
            
            i++;
            *port = string_to_int_num(&host_field_value[i]);
        }
    }

    free(host_field_value);
    return 1;
}

int get_field_value(char *full_data, char *field, char *storage)
{
    bool field_not_found = true;
    int request_size = strlen(full_data);
    bool match = true;

    for (int i = 0; i < request_size; i++) {
        if (full_data[i] == '\n') {
            match = true;
            for (int k = 0; k < strlen(field); k++) {
                if (full_data[i + k + 1] != field[k]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                /* i increased by strlen(field) + 3 to extract only the value 
                after 'FIELD:' */
                i = i + strlen(field) + 3;
                int j = 0;
                while (full_data[i] != '\n' && full_data[i] != '\r'){
                    storage[j] = full_data[i];
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

int string_to_int_num(char *content)
{
    int j = 0;
    int current_digit;
    int content_length = 0;

    int field_length = strlen(content);
    while (j < field_length) {
        current_digit = content[j] - '0';
        content_length+= current_digit * power(10, field_length - j - 1);
        j++;
    }

    return content_length;
}

int power(unsigned int a, unsigned int b)
{
    unsigned int answer = 1;
    if (b == 0) return answer;

    answer = a;
    for (unsigned int i = 1; i < b; i++) {
        answer *= a;
    }

    return answer;
}

int set_hostname(char *full_data, char *hostname)
{
    char *host_field_value = calloc(OBJECTNAMESIZE, sizeof(char));
    assert(host_field_value);
    int status = get_field_value(full_data, "Host", host_field_value);
    if (status == -1) {
        fprintf(stderr, "Invalid Request\n");
        free(host_field_value);
        return -1;
    }
    int field_length = strlen(host_field_value);

    /* strip port number */
    for (int i = 0; i < field_length; i++) {
        if (host_field_value[i] == ':') {
            host_field_value[i] = '\0';
            break;
        }
    }

    memset(hostname, '\0', strlen(hostname));
    strcpy(hostname, host_field_value);
    free(host_field_value);
    return 1;
}

bool CONNECT_request(char *full_data)
{
    if (full_data[0] == 'C' && full_data[1] == 'O' && full_data[2] == 'N' && 
    full_data[3] == 'N' && full_data[4] == 'E' && full_data[5] == 'C' && 
    full_data[6] == 'T')
        return true;

    return false;
}

/* returns -1 if something goes wrong */
int establish_connection_with_server(int *serverfd, client_request request)
{
    struct hostent *server;
    struct sockaddr_in server_addr;

    /* create a new socket on the proxy server for
    server connection */
    *serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverfd < 0) error("ERROR opening socket to server");
    server = gethostbyname(request->hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, host not found\n");
        return -1;
    }

    /* establish the server's Internet address  */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    /* copies h_addr (first address in the array of network addresses
    returned by gethostbyname()) to server_addr.sin_addr.s_addr */
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,
            server->h_length);
    server_addr.sin_port = htons(request->port);
    /* establish a connection to the server */
    if (connect(*serverfd, (struct sockaddr *) &server_addr, 
        sizeof(server_addr)) < 0) 
        return -1;
}

int total_response_size(char *object)
{
    char *response_header = calloc(HEADERSIZE, sizeof(char));
    int header_size = 0;
    int i = 0;
    int total_size = 0;

    while (1) {
        if (object[i] == '\n' &&
            object[i + 1] == '\r') {
            header_size = i;
            break;
        }
        response_header[i] = object[i];
        i++;
    }

    char *content = calloc(OBJECTNAMESIZE, sizeof(char));
    assert(content);
    int status = get_field_value(response_header, "Content-Length", content);
    if (status == -1) {
        fprintf(stderr, "Invalid Request\n");
        free(content);
        exit(1);
    }

    total_size = header_size + string_to_int_num(content) + SIZEPADDING;
    free(response_header);
    free(content);

    return total_size;
}

void send_HTTP_OK(int clientfd)
{
    int m;
    char *HTTP_OK = "HTTP/1.1 200 Connection Established\r\n\r\n";
    fprintf(stderr, "strlen(HTTP_OK)=%d\n", strlen(HTTP_OK));
    /* BANDWIDTH LIMIT Save */ 
    // commented out original
    limit_save(clientfd, HTTP_OK, strlen(HTTP_OK), false);
    /* ORIGINAL
    m = write(clientfd, HTTP_OK, strlen(HTTP_OK));
    
    if (m < 0) {
        close(clientfd);
    }
    */
    /* BANDWIDTH LIMIT Save */ 
    
    fprintf(stderr, "HTTP_OK=%s\n", HTTP_OK);
}

// char *get_url(char *HTTP_request)
// {
//     int starting_point;
//     int request_size = strlen(HTTP_request);
//     char *server_path = calloc(OBJECTNAMESIZE, sizeof(char));
//     assert(server_path);

//     for (int i = 4; i < request_size; i++) {
//         if (HTTP_request[i] == ' ') {
//             server_path[i - 4] = '\0';
//             break;
//         }
//         server_path[i - 4] = HTTP_request[i];
//     }  

//     return server_path;
// }

char *get_url(char *HTTP_request)
{
    int starting_point;
    int request_size = strlen(HTTP_request);
    char *server_path = calloc(OBJECTNAMESIZE, sizeof(char));
    assert(server_path);

    for (int j = 0; j < request_size; j++) {
        if (HTTP_request[j] == ' ') {
            starting_point = j + 1;
            break;
        }
    }

    for (int i = starting_point; i < request_size; i++) {
        if (HTTP_request[i] == ' ') {
            server_path[i - starting_point] = '\0';
            break;
        }
        server_path[i - starting_point] = HTTP_request[i];
    }  

    return server_path;
}

int connection_pair_method(int fd, connection *connections)
{
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] != NULL) {
            if (connections[i]->clientfd == fd || connections[i]->serverfd == fd) {
                return connections[i]->method;
            }
        }
    }
}

char *connection_pair_url(int fd, connection *connections)
{
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] != NULL) {
            if (connections[i]->clientfd == fd || connections[i]->serverfd == fd) {
                return connections[i]->url;
            }
        }
    }
}

/*********** for bandwidth, checks if the fd in connections is a client **********/

bool is_client(int fd, connection *connections)
{
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] != NULL) {
            if (connections[i]->clientfd == fd) {
                return true;
            }
        }
    }
    return false;
}

/*Code for Cache*/

Cache createCache(){
    Cache  new_cache =  malloc(sizeof(*new_cache));
    for(int i = 0; i < CACHE_SIZE; i++){
        new_cache->cache[i] = NULL;
    }
    return new_cache;

}

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

char* getFromCache(Cache cache, char* url){
    fprintf(stderr, "The url passed here is: %s\n", url);
     if(url == NULL ||  cache == NULL){
        assert("The parameters can not be null");}
    
    uint32_t hash_value = jenkinsHashFunction(url);
    uint32_t index = hash_value % CACHE_SIZE;
    
    if(isUrlPresent(url)){
        CacheBlock cblock = cache->cache[index];
        while(cblock != NULL && strcmp(cblock->url, url) != 0 ){
            cblock = cblock->next_element;
        }
        if (cblock != NULL){
            cblock->last_accessed = time(NULL);
            return cblock->content;
        }
    }
    
    
    return NULL;
}

/* returns -1 if url not found */
int content_size(Cache cache, char* url)
{
    fprintf(stderr, "The url passed here is: %s\n", url);
     if(url == NULL ||  cache == NULL){
        assert("The parameters can not be null");}
    
    if(isUrlPresent(url)){
        uint32_t hash_value = jenkinsHashFunction(url);
        uint32_t index = hash_value % CACHE_SIZE;
        
        CacheBlock cblock = cache->cache[index];
        while(cblock != NULL && strcmp(cblock->url, url) != 0 ){
            cblock = cblock->next_element;
        }
        if (cblock != NULL){
            return cblock->content_length;
    }
    }
    
    
    return -1;
}

void resizeCache(Cache *cache){
    printf("resizing the cache\n");
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

int isUrlPresent(char* url){
    int hash1 = hash_function1(url) % FILTER_SIZE;
    int hash2 = hash_function2(url) % FILTER_SIZE;
    int hash3 = hash_function3(url) % FILTER_SIZE;
    int flag = getBloomIndex(hash1) & getBloomIndex(hash2) & getBloomIndex(hash3);
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
    flag= flag << index;
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
