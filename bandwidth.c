#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <bandwidth.h>


#define SEND_SIZE 100000 // 3000k, 

#define MAX_SOCKET_NUM  30  
#define MAX_BUFFER_SIZE 10000000

/*
Algorithm: 
for each transfer, calculate next time the transfer should happen so that 
Max Bandwidth is kept every time we send

            send_data
next_time = --------- + first_time
            bandwidth
*/

typedef struct BandwidthBlock {
    int     write_address; // initialized to be 0, size of what should be sent
    int     send_size;
    double  last_time; // intitialized to be 0, time of what was last sent, identifies if everything was sent
    double  wait_time; // intitialized to be 0, wait time
    char *  content;     
    int     content_size;
} *BandwidthBlock;

typedef struct Bandwidth {
    BandwidthBlock blocks[MAX_SOCKET_NUM];
} *Bandwidth;

int max_bandwidth; // 0 if there is no bandwidth_limiting specified by client
Bandwidth bandwidth_blocks;

BandwidthBlock initiate_bandwidth();
double get_wait_time (int size);


/* getting wait time */
double get_wait_time (int size) {
    return ((double)size / (double)max_bandwidth);  
}

/******************* initializing the interface ******************/

BandwidthBlock initiate_bandwidth() {
    BandwidthBlock block = malloc(sizeof(*block));
    block->write_address = 0;
    block->send_size = 0;
    block->last_time = 0.0;
    block->wait_time = 0.0;
    block->content = malloc(sizeof(char) * MAX_BUFFER_SIZE);
    block->content_size = 0;
    return block;
}

// assumes that there is a bandwidth control if this function is called
void limit_set_bandwidth(int bandwidth) { 
    //fprintf(stderr, "bandwidth initialized\n");
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
    int m = 0;

    struct timeval curr;
    gettimeofday(&curr, NULL);
    double curr_time = (double)curr.tv_sec + (double)curr.tv_usec / 1000000.0;
    
    // when bandwidth = 0, no bandwidth control so send
    // when block->wait_time + block->last_time <= curr_time, bandwidth is set
    if (block->content_size > block->write_address) { 
        if (max_bandwidth == 0) {
            //fprintf(stderr, "send without bandwidth limit\n");
            m = write(fd, block->content, block->content_size);
            limit_clear(fd);
        } else {
            if (block->wait_time + block->last_time <= curr_time) {

                if (block->write_address + SEND_SIZE < block->content_size) 
                    block->send_size = SEND_SIZE;
                else 
                    block->send_size = block->content_size - block->write_address;
                //fprintf(stderr, "wait time is %f, ", block->wait_time);
                //fprintf(stderr, "send with bandwidth limit of size %d\n", block->send_size);
                m = write(fd, block->content + block->write_address, block->send_size);
                block->write_address += block->send_size;
                block->wait_time = get_wait_time(block->send_size);
                block->last_time = curr_time;                
            }
        }
    }
    return m;
}


/***************** clearing when disconnect/ all data sent ******************/

// resetting bandwidth information when connection closed
void limit_clear(int fd) {
    //fprintf(stderr, "limit clear called, ");
    BandwidthBlock block = bandwidth_blocks->blocks[fd];
    block->write_address = 0;
    block->send_size = 0;
    block->last_time = 0.0;
    block->wait_time = 0.0;
    bzero(block->content, MAX_BUFFER_SIZE);
    block->content_size = 0;
} 

/***************** saving data to send after read ******************/

void limit_read(int fd, char* data, int data_size, bool in_cache) {
    BandwidthBlock block = bandwidth_blocks->blocks[fd];
    fprintf(stderr, "fd is %d\n", fd);
    
    if (in_cache) {
        //fprintf(stderr, "saving limit of size %d in cache\n", data_size);
        memcpy(block->content, data, data_size);
        block->content_size = data_size;
    }
    else {
        //fprintf(stderr, "saving limit of size %d in not cache\n", data_size);
        memcpy(block->content + block->content_size, data, data_size);
        block->content_size += data_size;
    }
}






