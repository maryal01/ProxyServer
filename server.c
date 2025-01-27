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
#include "bandwidth.h"
#include "cache.h"
#include <sys/types.h>
#include <sys/stat.h>

/* pending connection queue's max length */
#define BACKLOG 20
#define BUFSIZE 1000000
/* max response (webpage) size */
#define OBJECTSIZE 20000000
/* size of cache */
#define CACHELINES 20
/* client request's single line max size */
#define OBJECTNAMESIZE 100
#define DEFAULTPORT 80
#define HEADERSIZE 64000
#define SIZEPADDING 10
#define CONCURRENTCONNECTIONS 40
#define GET 0
#define CONNECT 1


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
void print_active_fds(connection *connections);



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
            int fd = get_socket_number(i);
            m = limit_write(fd);
            // if (m < 0) {

            //     fprintf(stderr, "ERROR writing from limiting bandwidth");
            //     // remove_connection_pair(fd, connections);
            //     close(fd);
            //     partnerfd = partner(fd, connections);
            //     close(partnerfd);
            //         // BANDWIDTH LIMIT CLEAR fd info ON ERROR
            //     limit_clear(fd);
            // }
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
                    m = read(i, buffer, BUFSIZE);
                    if (m <= 0) {
                        fprintf(stderr,"read 0 or less bytes\n");
                        // struct stat buf;
                        // if (fstat(i, &buf) == -1) {
                        //     fprintf(stderr, "fd=%d is closed or not accessible\n", i);
                        // }
                        // else {
                        //     fprintf(stderr, "fd=%d is accessible\n", i);
                        // }
                         
                        if (connection_exists(i, connections)) {

                            // print_active_fds(connections);
                            // if( connection_pair_url(i, connections) == NULL){
                            //     fprintf(stderr, "FOUND THE BUG IN PROXY!!! \n");
                            // } else {
                            //      fprintf(stderr, "CONNECTION PAIR IS NOT NULL \n");
                            // }
                            
                            //signifying end of tcp streaming
                            if (m == 0) {
                                if(connection_pair_method(i, connections) == GET) {
                                    if (!is_client(i, connections)) {
                                        // fprintf(stderr, "problem caused inside m == 0 inserToCache\n");
                                        insertToCache(proxyCache, connection_pair_url(i, connections), NULL, -107);
                                    }
                                }
                            }

                            
                            partnerfd = partner(i, connections);
                            if (partnerfd != server) {
                                close(partnerfd);
                                FD_CLR(partnerfd, &active_fd_set);
                            }
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
                            limit_read(partnerfd, buffer, m, false);
                        
                        // simply write to server (e.g. facebook.com)
                        // if partnerfd is a server
                        else {
                            m = write(partnerfd, buffer, m);
                            if (m < 0) {
                                remove_connection_pair(i, connections);
                                // fprintf(stderr, "removed pair of %d and %d inside resource NOT found in 1\n", i, partnerfd);
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
                            // fprintf(stderr, "created pair of %d and %d 1\n", i, serverfd);
                            add_fd(i);
                            send_HTTP_OK(i);
                        } else {
                            fprintf(stderr, "GET request\n");
                            objectFromCache = getFromCache(proxyCache, url);
                            size = content_size(proxyCache, url);
                            if (objectFromCache) {
                                create_connection_pair(i, server, GET, url, connections);
                                // fprintf(stderr, "created pair of %d and %d 2\n", i, server);
                                fprintf(stderr, "resource found in cache \n");
                                /* BANDWIDTH LIMIT SAVE (commented out original) */
                                add_fd(i);
                                limit_read(i, objectFromCache, size, true);
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
                                // fprintf(stderr, "created pair of %d and %d 3\n", i, serverfd);
                                add_fd(i);
                                m = write(serverfd, buffer, strlen(buffer));
                                if (m < 0) {
                                    free(request);
                                    remove_connection_pair(serverfd, connections);
                                    close(i);
                                    partnerfd = partner(i, connections);
                                    close(partnerfd);
                                    // fprintf(stderr, "removed pair of %d and %d 2\n", i, partnerfd);
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
    // fprintf(stderr, "creating connection pair\n");
    connection tmp = malloc(sizeof(*tmp));
    assert(tmp);
    tmp->clientfd = clientfd;
    tmp->serverfd = serverfd;
    tmp->method = method;
    tmp->url = malloc(strlen(url) + 1);
    memcpy(tmp->url, url, strlen(url) + 1);
    // if (!tmp->url)
    //     printf("tmp->url is NULL\n");
    // else
    //     printf("tmp->url is %s\n", tmp->url);
//    tmp->url = url; //change if doesn't work: TODO

    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] == NULL) {
            connections[i] = tmp;
            return;
        }
    }
    // fprintf(stderr, "creating connection pair 2\n");

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
    m = write(clientfd, HTTP_OK, strlen(HTTP_OK));
    if (m < 0) 
        close(clientfd);
    
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

    // if (!server_path)
    //     fprintf(stderr, "server_path is NULL !!!\n");
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
    // fprintf(stderr, "entered connection_pair_url()\n");
    for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
        if (connections[i] != NULL) {
            if (connections[i]->clientfd == fd || connections[i]->serverfd == fd) {
                // fprintf(stderr, "connections[i]->url=%s\n", connections[i]->url);
                return connections[i]->url;
                
            }
        }
    }
    // fprintf(stderr, "connection_pair_url returned NULL on fd=%d\n", fd);
    
    return NULL;

}

/* TODO: delete this test function */
// void print_active_fds(connection *connections)
// {
//     for (int i = 0; i < CONCURRENTCONNECTIONS; i++) {
//         if (connections[i] != NULL) {
//                 fprintf(stderr, "%d %d ", connections[i]->serverfd, connections[i]->clientfd);
//         }
//     }
//     fprintf(stderr, "\n");
// }

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
