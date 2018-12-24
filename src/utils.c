#include <stdio.h> // sprintf and NULL

#include "utils.h"

void format_mac_addr(char *data, char *formatted) {
  sprintf(formatted, "%02x:%02x:%02x:%02x:%02x:%02x\0",
         (unsigned char) data[0],
         (unsigned char) data[1],
         (unsigned char) data[2],
         (unsigned char) data[3],
         (unsigned char) data[4],
         (unsigned char) data[5]);
}


char* advance_pos(struct packet *packet, int n_bytes) {
  if (n_bytes <= packet->remaining_length) {
    char *curr_pos = packet->pos;
    packet->pos += n_bytes;
    packet->remaining_length -= n_bytes;
    return curr_pos;
  }
  return NULL;
}
