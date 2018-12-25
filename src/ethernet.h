#ifndef ETHERNET_H
#define ETHERNET_H

/*
  args: the packet to parse
  
  This will extrac the ethernet header from the packet
  to determine which protocol is used. It will then call the
  next appropriate parsing function.

  Returns 0 on success and -1 on failure.
*/

int parse_ethernet_frame(struct packet *packet);


/*
  args: the ethernet header

  This will print the information stored in the header.
*/

void print_ethernet_frame(struct ethhdr *eth_header);


#endif/* ETHERNET_H */
