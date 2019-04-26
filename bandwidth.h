#ifndef BANDWIDTH_INCLUDED
#define BANDWIDTH_INCLUDED

void set_max_bandwidth(int bandwidth);
bool is_limit_sending(int fd);
int limit_uncached(int fd, char *data, int data_size);
int limit_cached(int fd, char *cached_data, int cached_data_size);
void clear_bandwidth(int fd);

#endif
