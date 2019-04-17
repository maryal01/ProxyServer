/*
    Bakar Tavadze
    btavad01

    COMP 112 - A1: HTTP Proxy
    last edit: 2/8/2019

    NOTES: 

    "server", mentioned in comments and code, is different from 
    "proxy server". "server" is the final destination the client is trying to 
    access (http://example.com)
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

#define BUFSIZE 2048
#define CONNECTIONS 5
#define SERVERPORT 80
#define OBJECTSIZE 10000000
#define OBJECTNAMESIZE 100
#define CACHELINES 10
#define HEADERSIZE 64000
#define SIZEPADDING 10
#define MAXAGE 1

typedef struct cache_line {
        char *object;
        double max_age;
        unsigned int recency_index;
        time_t stored_t;
        char name[OBJECTNAMESIZE];
              
} *cache_line;

void error(char *msg) {
    perror(msg);
    exit(1);
}

void set_hostname(char *HTTP_request, char *hostname);
void set_portno(char *HTTP_request, int *portno);
int power(unsigned int a, unsigned int b);
int get_field_value(char *HTTP_request, char *field, char *storage);
char *get_server_path(char *HTTP_request);
char *get_cached_object(char *request, cache_line *cache, time_t request_t);
char *retrieve_from_server(char *request);
void setup_cache(cache_line *cache);
int cache_line_index(char *name, cache_line *cache);
int total_response_size(char *object);
int find_free_line(cache_line *cache);
char *insert_field_in_header(char *field_name, double val, int size, 
                                char *trimmed_object);
int string_to_int_num(char *content);
void increase_indices_above(cache_line *cache, int n);
void increase_indices_below(cache_line *cache, int n);
int least_recently_accessed(cache_line *cache);
void create_cache_line(cache_line *cache, int index, char *server_path, 
                        char *object, int max_age, unsigned int recency_index);
int stale_line(cache_line *cache);
bool fresh(cache_line *cache);

int main(int argc, char *argv[])
{
    int proxyfd, clientfd;
    int proxy_portno;
    int clientlen, n;
    char buffer[BUFSIZE];
    char *object_buffer;
    struct sockaddr_in proxy_addr, client_addr;
    cache_line cache[CACHELINES];
    time_t request_t;
  
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    setup_cache(cache);

    /* create a new socket on the proxy server for clients */
    proxyfd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxyfd < 0) 
        error("ERROR opening proxy socket to client"); 

    /* establish the proxy server's Internet address  */
    bzero((char *) &proxy_addr, sizeof(proxy_addr));
    proxy_portno = atoi(argv[1]);
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(proxy_portno);

    /* link the socket with the proxy server's address */
    if (bind(proxyfd, (struct sockaddr *) &proxy_addr,
              sizeof(proxy_addr)) < 0) 
              error("ERROR on binding");
    /* ready for connection requests */
    listen(proxyfd, CONNECTIONS);

    clientlen = sizeof(client_addr);
    /* handle one client at a time, continually */
    
    while(1) {
        /* wait for a connection request */
        clientfd = accept(proxyfd, (struct sockaddr *) &client_addr, 
                            &clientlen);
        if (clientfd < 0) error("ERROR on client accept");

        int index = 0;
        bzero(buffer, BUFSIZE);
        n = read(clientfd, buffer, BUFSIZE);
        if (n < 0) error("ERROR reading from client socket");

        time(&request_t);
        char *cached_object = get_cached_object(buffer, cache, request_t);
        int size = total_response_size(cached_object);

        n = write(clientfd, cached_object, size);
        if (n < 0) error("ERROR writing to client socket");

        free(cached_object);
        close(clientfd);
    }

    close(proxyfd);
    return 0;
}

void set_portno(char *HTTP_request, int *portno)
{
    char *host_field_value = calloc(OBJECTNAMESIZE, sizeof(char));
    assert(host_field_value);
    int status = get_field_value(HTTP_request, "Host", host_field_value);
    if (status == -1) {
        fprintf(stderr, "Invalid Request\n");
        free(host_field_value);
        exit(1);
    }
    int field_length = strlen(host_field_value);
    int current_digit;

    for (int i = 0; i < field_length; i++) {
        if (host_field_value[i] == ':') {
            
            i++;
            *portno = string_to_int_num(&host_field_value[i]);
        }
    }

    free(host_field_value);
}

void set_hostname(char *HTTP_request, char *hostname)
{
    char *host_field_value = calloc(OBJECTNAMESIZE, sizeof(char));
    assert(host_field_value);
    int status = get_field_value(HTTP_request, "Host", host_field_value);
    if (status == -1) {
        fprintf(stderr, "Invalid Request\n");
        free(host_field_value);
        exit(1);
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
}

char *get_server_path(char *HTTP_request)
{
    int request_size = strlen(HTTP_request);
    char *server_path = calloc(OBJECTNAMESIZE, sizeof(char));
    assert(server_path);

    for (int i = 4; i < request_size; i++) {
        if (HTTP_request[i] == ' ') {
            server_path[i - 4] = '\0';
            break;
        }
        server_path[i - 4] = HTTP_request[i];
    }  

    return server_path;
}

int get_field_value(char *HTTP_request, char *field, char *storage)
{
    bool field_not_found = true;
    int request_size = strlen(HTTP_request);
    bool match = true;

    for (int i = 0; i < request_size; i++) {
        if (HTTP_request[i] == '\n') {
            match = true;
            for (int k = 0; k < strlen(field); k++) {
                if (HTTP_request[i + k + 1] != field[k]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                /* i increased by strlen(field) + 3 to extract only the value 
                after 'FIELD:' */
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

char *get_cached_object(char *request, cache_line *cache, time_t request_t)
{   
    char *requested_obj;
    char *trimmed_object;
    char *final_object;
    int p, size;
    double age = 0;
    bool straight_from_server = true;
    int free_line, max_age = MAXAGE * 3600;

    char *server_path = get_server_path(request);
    p = cache_line_index(server_path, cache);
    
    /* not in cache */
    if (p == -1) {
        requested_obj = retrieve_from_server(request);
        size = total_response_size(requested_obj);
        trimmed_object = malloc(size * sizeof(char));
        memcpy(trimmed_object, requested_obj, size);
        free(requested_obj);

        char *max_age_field_value = calloc(OBJECTNAMESIZE, sizeof(char));
        assert(max_age_field_value);
        int status = get_field_value(trimmed_object, "Cache-Control: max-ag", 
                                        max_age_field_value);

        if (status != -1) {
            max_age = string_to_int_num(max_age_field_value);
        }
        free(max_age_field_value);

        free_line = find_free_line(cache);
        /* cache full */
        if (free_line == -1) {
            int stale = stale_line(cache);
            /* all fresh */
            if (stale == - 1) {
                int k = least_recently_accessed(cache);
                increase_indices_above(cache, 1);
                free(cache[k]->object);
                free(cache[k]);
                create_cache_line(cache, k, server_path, trimmed_object, 
                                max_age, 1);
            }
            /* stale found */
            else {
                increase_indices_below(cache, cache[stale]->recency_index);
                free(cache[stale]->object);
                free(cache[stale]);
                create_cache_line(cache, stale, server_path, trimmed_object, 
                                max_age, 1);
            }
        }
        /* there's space in cache */
        else {
            increase_indices_above(cache, 1);
            create_cache_line(cache, free_line, server_path, trimmed_object, 
                                max_age, 1);               
        }
       
    }
    /* in cache */
    else {
        if (fresh(&cache[p])) {
            trimmed_object = cache[p]->object;
            size = total_response_size(trimmed_object);
            increase_indices_below(cache, cache[p]->recency_index);
            cache[p]->recency_index = 1;
            time_t current_t;
            time(&current_t);
            age = difftime(current_t, cache[p]->stored_t);
            straight_from_server = false;

        }
        else {
            requested_obj = retrieve_from_server(request);
            size = total_response_size(requested_obj);
            trimmed_object = malloc(size * sizeof(char));
            memcpy(trimmed_object, requested_obj, size);
            free(requested_obj);

            char *max_age_field_value = calloc(OBJECTNAMESIZE, sizeof(char));
            assert(max_age_field_value);
            int status = get_field_value(trimmed_object, 
                                        "Cache-Control: max-ag", 
                                        max_age_field_value);

            if (status != -1) {
                max_age = string_to_int_num(max_age_field_value);
            }
            
            free(max_age_field_value);

            increase_indices_below(cache, cache[p]->recency_index);
            free(cache[p]->object);
            cache[p]->object = trimmed_object;
            time(&(cache[p]->stored_t));
            cache[p]->max_age = max_age;
            cache[p]->recency_index = 1;
        }
    }

    free(server_path);
    
    if(straight_from_server) {
        final_object = malloc(size * sizeof(char));
        memcpy(final_object, trimmed_object, size);
    }
    else {
        final_object = insert_field_in_header("Age: ", age, size, trimmed_object);
    }
    
    
    return final_object;
}

void create_cache_line(cache_line *cache, int index, char *server_path, 
                        char *object, int max_age, unsigned int recency_index)
{
    cache_line line = calloc(sizeof(*line), sizeof(char));
    memcpy(line->name, server_path, strlen(server_path));

    line->object = object;
    time(&(line->stored_t));
    line->max_age = max_age;
    line->recency_index = recency_index;
    cache[index] = line;
}

int stale_line(cache_line *cache)
{
    int index = -1;

    for (int i = 0; i < CACHELINES; i++) {
        assert(cache[i]);
        if (!fresh(&cache[i])) {
            index = i;
            break;
        }

    }

    return index;
}

bool fresh(cache_line *cache) 
{
    double max_age = (*cache)->max_age;
    time_t stored_t = (*cache)->stored_t;
    time_t current_t;
    time(&current_t);

    if (difftime(current_t, stored_t) > max_age) 
        return false;
    else
        return true;
}

void increase_indices_above(cache_line *cache, int n) 
{
    for (int i = 0; i < CACHELINES; i++) {
        if (cache[i] != NULL) {
            if (cache[i]->recency_index >= n)
                cache[i]->recency_index++;
        }
    }
}

void increase_indices_below(cache_line *cache, int n) 
{
    for (int i = 0; i < CACHELINES; i++) {
        if (cache[i] != NULL) {
            if (cache[i]->recency_index < n)
                cache[i]->recency_index++;
        }
    }
}

int least_recently_accessed(cache_line *cache)
{
    int index = -1;

    for (int i = 0; i < CACHELINES; i++) {
        assert(cache[i]);
        if (cache[i]->recency_index == CACHELINES) {
            index = i;
            break;
        }
    }

    return index;
}

/* "size" is the total object size */
char *insert_field_in_header(char *field_name, double val, int size, 
                                char *trimmed_object)
{
    char field[OBJECTNAMESIZE];
    sprintf(field, "%s%.0f\n", field_name, val);
    int field_length = strlen(field);
    char *final_respose = malloc((size + field_length + 1) * sizeof(char));
    int first_line_end;

    for (int i = 0; i < size; i++) {
        final_respose[i] = trimmed_object[i];
        if (trimmed_object[i] == '\n') {
            first_line_end = i;
            break;
        }
    }
    
    memcpy(&(final_respose[first_line_end + 1]), field, field_length);
    memcpy(&(final_respose[first_line_end + 1 + field_length]), 
        &(trimmed_object[first_line_end + 1]), size - first_line_end - 1);

    return final_respose;
}

void setup_cache(cache_line *cache)
{
    for (int i = 0; i < CACHELINES; i++) {
        cache[i] = NULL;
    }
}

/* -1 indicates cache is full */
int find_free_line(cache_line *cache)
{ 
    int index = -1;

    for (int i = 0; i < CACHELINES; i++) {
        if (cache[i] == NULL) {
            index = i;
            break;
        }
    }

    return index;
}

/* -1 indicates file wasn't found */
int cache_line_index(char *name, cache_line *cache)
{
    int index = -1;

    for (int i = 0; i < CACHELINES; i++) {
        if (cache[i] != NULL) {
            int res = strcmp(cache[i]->name, name);
            if (res == 0) {
                index = i;
                break;
            }
        }
    }

    return index;
}

char *retrieve_from_server(char *request)
{
    char hostname[OBJECTNAMESIZE] = "";
    int server_portno = SERVERPORT;
    int serverfd;
    struct hostent *server;
    struct sockaddr_in server_addr;
    char buffer[BUFSIZE];
    char *object_buffer;
    int m;

    set_hostname(request, hostname);
    set_portno(request, &server_portno);

    /* create a new socket on the proxy server for
    server connection */
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) error("ERROR opening socket to server");
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, host not found\n");
        exit(1);
    }

    /* establish the server's Internet address  */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    /* copies h_addr (first address in the array of network addresses
    returned by gethostbyname()) to server_addr.sin_addr.s_addr */
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr,
            server->h_length);
    server_addr.sin_port = htons(server_portno);
    /* establish a connection to the server */
    if (connect(serverfd, (struct sockaddr *) &server_addr, 
        sizeof(server_addr)) < 0) error("ERROR connecting to the server");
    
    m = write(serverfd, request, strlen(request));
    if (m < 0) error("ERROR writing to server socket");
    /* allocate memory to read/write large HTTP objects */
    object_buffer = malloc(OBJECTSIZE * sizeof(char));
    assert(object_buffer);

    char *HTTP_response = malloc(OBJECTSIZE * sizeof(char));
    assert(HTTP_response);
    bzero(HTTP_response, OBJECTSIZE);

    int index = 0;
    bzero(object_buffer, OBJECTSIZE);
    m = read(serverfd, object_buffer, OBJECTSIZE);
    if (m < 0) error("ERROR reading from server socket");
    while (m > 0) {
        memcpy(&HTTP_response[index], object_buffer, m);
        index += m;
        m = read(serverfd, object_buffer, OBJECTSIZE);    
    }

    free(object_buffer);

    return HTTP_response;
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
