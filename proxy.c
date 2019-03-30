/*
    TODO: remove 'exit()'s and 'error()'s to allow concurrent user access

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

/* pending connection queue's max length */
#define BACKLOG 20
#define BUFSIZE 2048
/* max response (webpage) size */
#define OBJECTSIZE 10000000
/* size of cache */
#define CACHELINES 20
/* client request's single line max size */
#define OBJECTNAMESIZE 100
#define DEFAULTPORT 80
#define HEADERSIZE 64000
#define SIZEPADDING 10

/* method can be either GET or CONNECT
   full_data is the full request message from client
*/
typedef struct client_request {
        char method[10];
        char hostname[OBJECTNAMESIZE]; 
        int port;
        char *full_data;
} *client_request;

/* single cache line */
typedef struct cache_line {
        char *object;
        /* etc ... */
} *cache_line;

fd_set active_fd_set, read_fd_set;

void error(char *msg);
void ensure_argument_validity(int argc, char *executable_name);
int create_socket(uint16_t port);
void setup_cache(cache_line *cache);
int get_response(char **response, char *full_data, cache_line *cache);
int build_client_request(char *full_data, client_request *request);
int set_portno(char *full_data, int *port);
int set_hostname(char *full_data, char *hostname);
int get_field_value(char *HTTP_request, char *field, char *storage);
int string_to_int_num(char *content);
int power(unsigned int a, unsigned int b);
int retrieve_from_server(char *full_data, client_request request, char **response);
int total_response_size(char *object);

int main(int argc, char *argv[])
{
    int i, m;
    int server, port;
    int new_client, client_size;
    struct sockaddr_in clientname;
    struct timeval tv;
    char buffer[BUFSIZE];
    char *response;
    int response_size;
    cache_line cache[CACHELINES];

    ensure_argument_validity(argc, argv[0]);
    port = atoi(argv[1]);
    server = create_socket(port);
    if (listen (server, BACKLOG) < 0) 
        error("listen");

    FD_ZERO (&active_fd_set);
    FD_SET (server, &active_fd_set);

    setup_cache(cache);

    while (true) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL,  &tv) < 0)
            error("select");

        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == server)
                {
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
                    bzero(buffer, BUFSIZE);
                    m = read(i, buffer, BUFSIZE);
                    if (m <= 0) {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                        continue;
                    }

                    m = get_response(&response, buffer, cache);
                    if (m == -1) {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                    } else {
                        response_size = total_response_size(response);
                        m = write(i, response, response_size);
                        if (m < 0) error("ERROR writing to client");
                    }
                    if (response)
                            free(response);
                    
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
void ensure_argument_validity(int argc, char *executable_name)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", executable_name);
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

/* if something went wrong returns -1 and sets response to NULL */
int get_response(char **response, char *full_data, cache_line *cache)
{
    int status_code;
    char *server_response;
    client_request request;

    status_code = build_client_request(full_data, &request);
    if (status_code == -1) {
        free(request);
        return -1;
    }

    status_code = retrieve_from_server(request->full_data, request, &server_response);
    if (status_code == -1) {
        if (server_response)
            free(server_response);
        return -1;
    }

    free(request);

    *response = server_response;
    return 1;
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
    bzero(&(tmp->method), 10);
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


int retrieve_from_server(char *full_data, client_request request, char **response)
{
    int serverfd;
    struct hostent *server;
    struct sockaddr_in server_addr;
    char buffer[BUFSIZE];
    char *object_buffer;
    int m;

    /* create a new socket on the proxy server for
    server connection */
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) error("ERROR opening socket to server");
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
    if (connect(serverfd, (struct sockaddr *) &server_addr, 
        sizeof(server_addr)) < 0) error("ERROR connecting to the server");
    
    m = write(serverfd, full_data, strlen(full_data));
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

    *response = HTTP_response;
    return 1;
}

/* if something goes wrong returns size 0 */
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
        return 0;
    }

    total_size = header_size + string_to_int_num(content) + SIZEPADDING;
    free(response_header);
    free(content);

    return total_size;
}