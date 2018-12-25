#include <linux/if_ether.h> // ethhdr

#include "common.h"
#include "utils.h"
#include "ethernet.h"
#include "arp.h"


int parse_ethernet_frame(struct packet *packet) {
  // Check if there is enough data for an ethernet header.
  char *x = advance_pos(packet, sizeof(struct ethhdr));
  if(x == NULL) {
    printf("Not enough bytes for an ethernet header\n");
    return -1;
  }
  // Cast the captured data to an ethernet frame header
  struct ethhdr *eth_header = (struct ethhdr *)x;
  // The potocol must be converted to host order (2 octets)
  eth_header->h_proto = ntohs(eth_header->h_proto);

  switch(eth_header->h_proto) {
  case ETH_P_ARP:
    print_ethernet_frame(eth_header);
    printf("Received ARP message\n");
    parse_arp_message(packet);
    break;

  default :
    printf("Unsupported protocol type: %02x\n", eth_header->h_proto);
    break;
  }
  return 0;

}


void print_ethernet_frame(struct ethhdr *eth_header) {
  char dst_mac_addr[FORMATTED_MAC_ADDR_SIZE];
  char src_mac_addr[FORMATTED_MAC_ADDR_SIZE];
  format_mac_addr(eth_header->h_dest, dst_mac_addr);
  format_mac_addr(eth_header->h_source, src_mac_addr);
  printf("Destination: %s\nSource: %s\nType: %04x\n",
         dst_mac_addr, src_mac_addr, eth_header->h_proto);
}
