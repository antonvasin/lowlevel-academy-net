#ifndef SRVPOLL_H
#define SRVPOLL_H

#include "common.h"

#define MAX_CLIENTS 256

void init_clients(clientstate_t *states);
int find_free_slot(clientstate_t *states);
int find_slot_by_fd(int fd, clientstate_t *states);

#endif
