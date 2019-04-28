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


#define FROM_CACHE_SIZE 4000 
#define MAX_SOCKET_NUM  30  
#define MAX_CACHE_SIZE 10000000
#define MAX_BUFFER_SIZE 1000000

/*
Algorithm: 
for each transfer, calculate next time the transfer should happen so that 
Max Bandwidth is kept every time we send

            send_data
next_time = --------- + first_time
            bandwidth
*/

typedef struct BandwidthBlock {
    int     size; // initialized to be 0, size of what should be sent
    time_t  last_time; // intitialized to be 0, time of what was last sent, identifies if everything was sent
    int     wait_time; // intitialized to be 0, wait time
    bool    in_cache; // checks if its in cache
    char *  data;     // data of whats going to be sent || stores everything from the cache
    int     address; // for cache, address of up till where data is sent total
    int     cache_size; // for cache, size of content_size in cache
} *BandwidthBlock;

typedef struct Bandwidth {
    BandwidthBlock blocks[MAX_SOCKET_NUM];
} *Bandwidth;

int max_bandwidth; // 0 if there is no bandwidth_limiting specified by client
Bandwidth bandwidth_blocks;

int get_wait_time (int size);
BandwidthBlock initiate_bandwidth();
int write_cache (int fd, time_t curr_time, BandwidthBlock block);
int write_uncache (int fd, time_t curr_time, BandwidthBlock block);
void update_cache(time_t curr_time, BandwidthBlock block);
void update_uncache(time_t curr_time, BandwidthBlock block);
void save_cache(int fd, time_t curr_time, char *content, int content_size);
void save_uncache(int fd, time_t curr_time, char *data, int data_size);


int get_wait_time (int size) {
    return (size / max_bandwidth);  
}

/******************* initializing the interface ******************/

BandwidthBlock initiate_bandwidth() {
    BandwidthBlock new_block = malloc(sizeof(*new_block));
    new_block->size = 0;
    new_block->last_time = 0;
    new_block->wait_time = 0;
    new_block->in_cache = false;
    new_block->data = NULL;
    new_block->address = 0;
    new_block->cache_size = 0;
    return new_block;
}

// assumes that there is a bandwidth control if this function is called
void limit_set_bandwidth(int bandwidth) { 
    fprintf(stderr, "bandwidth initialized\n");
    max_bandwidth = bandwidth; // bits per second to bytes per second
    bandwidth_blocks = malloc(sizeof(*bandwidth_blocks));

    for(int i = 0; i < MAX_SOCKET_NUM; i++){
        bandwidth_blocks->blocks[i] = initiate_bandwidth();
    }
}

/******************* writing ******************/

// in proxy, will happen for each of the 20 sockets

int limit_write (int fd) {
    BandwidthBlock block = bandwidth_blocks->blocks[fd];
    time_t curr_time = time(NULL);
    if (block->wait_time + block->last_time <= curr_time && block->last_time != 0) {
        // for fd's that are not connected, there will be simply no writing, returning m <= 0
        if (block->in_cache)  {
            fprintf(stderr, "write, curr_time is %d", curr_time);
            return write_cache(fd, curr_time, block);
        }
        else 
            return write_uncache(fd, curr_time, block);
    } 
    return 0; // no data to write
}


// writing from cache
int write_cache (int fd, time_t curr_time, BandwidthBlock block) {
    fprintf(stderr, "writing address %d from cache\n", block->address);
    int m = write (fd, block->data + block->address, block->size);
    // not all data are sent
    if (block->address + block->size < block->cache_size)
        update_cache(curr_time, block);
    // all data are sent
    else  {
        fprintf(stderr, "writing address %d from cache\n", block->address);
        limit_clear(fd);
    }
    return m;
}

// writing from connections
int write_uncache (int fd, time_t curr_time, BandwidthBlock block) {
    fprintf(stderr, "writing from uncache of size %d\n", block->size);
    int m = write (fd, block->data, block->size);

    // disconnect from error flag of write raised;
    if (m < 0)
        limit_clear(fd);
    else 
        update_uncache(curr_time, block);
    return m;
}

/* 
when updating cache, keep whats stored in block->data, 
since all the content in cache are stored in block->data
*/
void update_cache(time_t curr_time, BandwidthBlock block) {
    fprintf(stderr, "updating cache\n");
    block->last_time = curr_time;
    if (block->cache_size - block->address > FROM_CACHE_SIZE)
        block->size = FROM_CACHE_SIZE;
    else
        block->size = block->cache_size - block->address;
    block->address += block->size;
    block->wait_time = get_wait_time(block->size);
}

/* 
when updating uncache, bzero out block->data , 
since only what we read from the server is stored in block->data
*/
void update_uncache(time_t curr_time, BandwidthBlock block) {
    fprintf(stderr, "updating uncache\n");
    block->size = 0;
    block->last_time = curr_time;
    block->wait_time = 0;
    bzero(block->data, MAX_BUFFER_SIZE);
}


/***************** clearing when disconnect/ all data sent ******************/

// resetting bandwidth information when connection closed
void limit_clear(int fd) {
    fprintf(stderr, "limit clear called, ");
    BandwidthBlock block = bandwidth_blocks->blocks[fd];
    if (block->data) {
        fprintf(stderr, "limit clear on broken connection\n");
        block->size = 0;
        block->last_time = 0;
        block->wait_time = 0;
        block->in_cache = false;
        free(block->data);
        block->data = NULL;
        block->address = 0;
        block->cache_size = 0;
    }
} 




/***************** saving data to send after read ******************/

void limit_save(int fd, char* data, int size, bool in_cache) {
    fprintf(stderr, "saving limit of size %d\n", size);
    BandwidthBlock block = bandwidth_blocks->blocks[fd];
    time_t curr_time = time(NULL);
    if (in_cache) {
        save_cache(fd, curr_time, data, size);
    }
    else {
        // if first time sending
        save_uncache(fd, curr_time, data, size);
    }
}

/*
Parameter:  int fd, time_t curr_time, char *content, int content_size
            content : total cache data (max size 10000000)
            content_size : total cache content size
Purpose:    saves cache content that has to be sent to fd with cache control
                - whole content from cache is memcpy'ed to BandwidthBlock->data.
                  therefore, calling this function will set everything up to be 
                  written to client requesting data from cache

*/
void save_cache(int fd, time_t curr_time, char *content, int content_size) {
    fprintf(stderr, "save cache\n");
    BandwidthBlock block = bandwidth_blocks->blocks[fd];
    block->data = malloc(sizeof(char) * MAX_CACHE_SIZE);
    memcpy(block->data, content, content_size);
    block->in_cache = true;
    block->cache_size = content_size;
    block->address = 0;

    // if no bandwidth control, send everything right away
    if (max_bandwidth == 0)
        block->size = content_size;

    // if there is bandwith control and cache_size > FROM_CACHE_SIZE 
    // send 400
    else if (content_size > FROM_CACHE_SIZE)
        block->size = FROM_CACHE_SIZE;
    // if there is bandwith control and cache_size <= FROM_CACHE_SIZE 
    // send only cache_size
    else
        block->size = content_size;

    // if there is no bandwidth control there should be no waiting
    if (max_bandwidth > 0) {
        block->wait_time = get_wait_time(block->size);
        fprintf(stderr, "waittime (cache) is %d, curr_time is %d\n", block->wait_time, curr_time);
    }
    else  {
        fprintf(stderr, "waittime (cache) is zero \n");
        block->wait_time = 0;
    }
    block->last_time = curr_time;
}

/*
Parameter:  int fd, time_t curr_time, char *content, int content_size
            data: data you read from server (e.g. facebook.com)
            content_size: size of data read from server (e.g. facebook.com)
Purpose:    saves content read from server that client requested data from
                - every time the proxy server reads new data from server (e.g. facebook.com)
                  this function should be called, 
*/
void save_uncache(int fd, time_t curr_time, char *data, int data_size) {
    fprintf(stderr, "saving uncache of size %d,", data_size);
    BandwidthBlock block = bandwidth_blocks->blocks[fd];

    if(block->last_time == 0)
        block->data = malloc(sizeof(char) * MAX_BUFFER_SIZE);
    else 
        bzero(block->data, MAX_BUFFER_SIZE);
    memcpy(block->data + block->size, data, data_size);

    block->size += data_size;
    block->in_cache = false;

    if (max_bandwidth > 0) {
        block->wait_time = get_wait_time(block->size);
        fprintf(stderr, "waittime is %d, curr_time is %d\n", block->wait_time, curr_time);
    }
    // if there is no bandwidth control there should be no waiting
    else  {
        fprintf(stderr, "waittime is zero \n");
        block->wait_time = 0;
    }

    // meaning this connection has never sent a data yet,
    // else last_time is updated in update_cache
    if(block->last_time == 0)
        block->last_time = curr_time;
}



/* controlling read for bandwidth control */
bool limit_read_wait(int fd) {
    BandwidthBlock block = bandwidth_blocks->blocks[fd];

    // if true, I have something waiting to be sent
    if (block->size != 0) {
        fprintf(stderr, "should wait \n");
        return true;
    }
    fprintf(stderr, "should not wait \n");
    return false;
}
