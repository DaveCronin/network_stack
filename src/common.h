#ifndef COMMON_H
#define COMMON_H

#include <stdio.h> // printf, snprintf etc
#include <errno.h>
#include <string.h> // strncpy
#include <arpa/inet.h> // hton, ntohs, ntoa, in_addr
#include <assert.h>
#include <stdbool.h> // can't believe this needs a .h
#include <stdlib.h> // malloc
#define ETHER_FAMILY 1
#define MAC_ADDR_SIZE 6

#define IPV4_ADDR_SIZE 4

#endif /* COMMON_H */
