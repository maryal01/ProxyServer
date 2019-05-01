#ifndef BANDWIDTH_INCLUDED
#define BANDWIDTH_INCLUDED

/*
typedef struct BandwidthBlock {
    int     size; 
    time_t  last_time; 
    int     wait_time; 
    bool    in_cache; 
    char *  data;     
    int     address; 
    int     cache_size; 
} *BandwidthBlock;
*/

/* 
parameter:  bandwidth
Purpose:	sets bandwidth 
            	- if bandwidth = 0, normal write
            	- else intializes interface for bandwidth control
*/
void limit_set_bandwidth(int bandwidth);

/*
Parameter:  int fd, time_t curr_time, char *content, int content_size
            content : total cache data (max size 10000000) if in_cache = true;
                      data read from server (e.g. facebook.com) if in_cache = false;
            content_size : total cache content size if in_cache = true;
                           data size read from server (e.g. facebook.com) if in_cache = false;
Purpose:	saves data to be sent with or without bandwidth controls

*/
void limit_read(int fd, char* data, int size, bool in_cache);

/* 
parameter:  fd
Purpose:	sends limited write to provided fd
				- if the socket requested a data from cache,
				  write from cache 
				  	* All the cache content is memcpy'd into char *data in the limit_save_cache,
				  	  therefore, calling this function will send all cache to the client 
				  	  (with limits if bandwidth > 0 else without limits)
				- if the socket requested a data from uncached content,
				  write based on data written from server (e.g. facebook.com)
				  	* calling this function will send all the data
				  	  that is not sent back to client but read from server (e.g. facebook.com) 
				  	  (with limits if bandwidth > 0 else without limits) 
Output:     return m > 0 if more than 0 bytes are written
                   m = 0 if nothing was written
                   m < 0 if there was an error writing
*/
int limit_write (int fd);

/* 
parameter:  fd
Purpose:	remove the socket information if the socket is disconnected from the server.
*/
void limit_clear(int fd);
void limit_clear_write(int i);

void add_fd(int fd);

#endif
