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

// Does my logic, accounting for the roundtrip time, make sense?
// should i use millisecond instead of seconds?
#define FROM_CACHE_SIZE 400 // Question to ask: is it fine to use 400 as the packet size? researched
#define MAX_SOCKET_NUM  30  // Question to ask: if 20 is max select size, will there be a mapping?

/*
Algorithm: 
for each transfer, calculate next time the transfer should happen so that 
Max Bandwidth is kept every time we send

            send_data
next_time = --------- + first_time
            bandwidth
*/

typedef struct BandwidthBlock {
    int sent_size;     // records number of bits sent so far
    time_t first_time; // records first time data was sent to the socket number
    time_t next_time;  // records the next time the data should be sent
} *BandwidthBlock;

typedef struct Bandwidth {
    BandwidthBlock blocks[MAX_SOCKET_NUM];
} *Bandwidth;

int max_bandwidth; // 0 if there is no bandwidth_limiting specified by client
Bandwidth band_info;

int get_bandwidth(int sent_size, time_t first_time, time_t curr_time) {
    return sent_size / (first_time - curr_time);
}

time_t get_next_time (int sent_size, time_t first) {
    return (time_t) (sent_size / max_bandwidth) + first;  
}

// assumes that there is a bandwidth control if this function is called
void set_max_bandwidth(int bandwidth) { 
    fprintf(stderr, "Proxy initialized\n");
    max_bandwidth = bandwidth; // bits per second to bytes per second
    if(bandwidth > 0) {
        band_info = malloc(sizeof(*band_info));

        for(int i = 0; i < MAX_SOCKET_NUM; i++){
            // malloc block for each socket here
            BandwidthBlock new_block = malloc(sizeof(*new_block));
            band_info->blocks[i] = new_block;
        }
    }
}

int limit_uncached(int fd, char *data, int data_size) {
    // if max_bandwidth is not specified, simply do write
    if (max_bandwidth == 0) 
        return write(fd, data, data_size); 

    time_t curr_time = time(NULL);
    int m;

    // if first time sending to client,
    // simply transfer, saving necessary data for next transfers
    if (band_info->blocks[fd]->first_time == 0) {
        fprintf(stderr, "\nnew data sent at %d from server for first time\n", curr_time);
        m = write(fd, data, data_size);
        band_info->blocks[fd]->sent_size = data_size;   // saving sent_size
        band_info->blocks[fd]->first_time = curr_time;  // saving first_time
        band_info->blocks[fd]->next_time = get_next_time(data_size, curr_time); // saving next_time
        fprintf(stderr, "next time is %d\n\n", band_info->blocks[fd]->next_time);
    }

    // if not first time sending to client,
    // check 
    else if (curr_time >= band_info->blocks[fd]->next_time) {
        fprintf(stderr, "new data sent at %d from server for later time\n", curr_time);
        m = write(fd, data, data_size);
        band_info->blocks[fd]->sent_size += data_size;
        band_info->blocks[fd]->next_time = get_next_time(data_size, band_info->blocks[fd]->first_time);
        fprintf(stderr, "next time is %d\n\n", band_info->blocks[fd]->next_time);
    } else 
        m = 0; // telling proxy that I did not send any data because time has not met
               // socket closes only when m < 0 in proxy

    return m;
}

int limit_cached(int fd, char *data, int content_size) {
    // if max_bandwidth is not specified, simply do write
    if (max_bandwidth == 0) 
        return write(fd, data, content_size); 

    time_t curr_time = time(NULL);
    fprintf(stderr, "time is %d\n", curr_time);
    int m;

    // if first send, just write first 400 of cached
    //     save sent_size, first_time, and next_time
    if (band_info->blocks[fd]->first_time == 0) {
        fprintf(stderr, "new data sent at %d from cache for first time\n", curr_time);
        fprintf(stderr, "content_size is %d\n", content_size);
        m = write(fd, data, FROM_CACHE_SIZE);
        band_info->blocks[fd]->sent_size = FROM_CACHE_SIZE;   // saving sent_size
        band_info->blocks[fd]->first_time = curr_time;  // saving first_time
        band_info->blocks[fd]->next_time = get_next_time(FROM_CACHE_SIZE, curr_time); // saving next_time
        fprintf(stderr, "next time is %d\n\n", band_info->blocks[fd]->next_time);
    }

    // if not first
    //     if not all data in cache are sent, 
    //         if curr time >= next
    //             write + sent_size, save send_size and next_time

    else if (band_info->blocks[fd]->sent_size < content_size 
          && curr_time >= band_info->blocks[fd]->next_time) 
    {
        fprintf(stderr, "new data sent at %d from cache for later time\n", curr_time);
        fprintf(stderr, "content_size is %d\n", content_size);
        m = write(fd, data + band_info->blocks[fd]->sent_size, FROM_CACHE_SIZE);
        band_info->blocks[fd]->sent_size += FROM_CACHE_SIZE;   // saving sent_size
        band_info->blocks[fd]->next_time = get_next_time(band_info->blocks[fd]->sent_size, band_info->blocks[fd]->first_time); // saving next_time
        fprintf(stderr, "next time is %d\n\n", band_info->blocks[fd]->next_time);
    } else
        m = 0; // telling proxy that I did not send any data because time has not met or 
               // socket closes only when m < 0 in proxy

    return m;
}

// resetting bandwidth information when closed
void clear_bandwidth (int fd) {
    band_info->blocks[fd]->sent_size = 0;
    band_info->blocks[fd]->first_time = 0;
    band_info->blocks[fd]->next_time = 0;
}

// checking if the socket fd sending is delayed due to bandwidth controlling
bool is_limit_sending(int fd) {
    return max_bandwidth != 0 && band_info->blocks[fd]->first_time != 0;
}
