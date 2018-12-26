#include <linux/if_ether.h> // ethhdr
#include <unistd.h> // write?
#include "common.h"
#include "utils.h"
#include "ethernet.h"
#include "arp.h"

static int fd;
struct ethhdr *eth_header;

int parse_ethernet_frame(int tap_fd, struct packet *packet) {
  // Check if there is enough data for an ethernet header.
  
  char *x = advance_pos(packet, sizeof(struct ethhdr));
  if(x == NULL) {
    printf("Not enough bytes for an ethernet header\n");
    return -1;
  }
  // Cast the captured data to an ethernet frame header
  eth_header = (struct ethhdr *)x;
  // The potocol must be converted to host order (2 octets)
  eth_header->h_proto = ntohs(eth_header->h_proto);

  fd = tap_fd;
  
  switch(eth_header->h_proto) {
  case ETH_P_ARP:
    print_ethernet_frame(eth_header);
    printf("Received ARP message\n");
    return parse_arp_message(packet);

  default:
    printf("Unsupported protocol type: %02x\n", eth_header->h_proto);
    // clear the remainder of the packet
    advance_pos(packet, packet->remaining_length);
    return 0;
  }
}

int send_ethernet_frame(char *buf, int size) {
  char tmp[ETH_ALEN] = {0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB};
  memcpy(eth_header->h_dest, eth_header->h_source, ETH_ALEN);
  memcpy(eth_header->h_source, tmp, ETH_ALEN);
  eth_header->h_proto = htons(eth_header->h_proto);
  char *buf_to_send = malloc(sizeof(struct ethhdr)+size);
  memcpy(buf_to_send, eth_header, sizeof(struct ethhdr));
  memcpy(buf_to_send+sizeof(struct ethhdr), buf, size);
  write(fd, buf_to_send, sizeof(struct ethhdr)+size);
  
  /* Push a frame back through my stack to debug it
  struct packet p;
  p.pos = buf_to_send;
  p.remaining_length = sizeof(struct ethhdr)+size;
  parse_ethernet_frame(1, &p);
  */
  
}


void print_ethernet_frame(struct ethhdr *eth_header) {
  char dst_mac_addr[FORMATTED_MAC_ADDR_SIZE];
  char src_mac_addr[FORMATTED_MAC_ADDR_SIZE];
  format_mac_addr(eth_header->h_dest, dst_mac_addr);
  format_mac_addr(eth_header->h_source, src_mac_addr);
  printf("Destination: %s\nSource: %s\nType: %04x\n",
         dst_mac_addr, src_mac_addr, eth_header->h_proto);
}
