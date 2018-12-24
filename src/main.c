#include <stdio.h> // printf
#include <string.h> // strncpy
#include <unistd.h> // for close, read

#include <arpa/inet.h> // hton, ntoa, in_addr
#include <linux/if_ether.h> // ethhdr

#include "tuntap.h"
#include "utils.h"

int main() {
  int tap_fd;
  char tap_name[IFNAMSIZ];
  strcpy(tap_name, "tap44");
  // TAP interface flags with no protocol information header
  // The header is 4 bytes [2:2, flags:protocol]
  // I couldn't find any good info on what this is so
  // I am removing the excess bytes and will assume
  // ethernet
  tap_fd = tuntap_alloc(tap_name, IFF_TAP | IFF_NO_PI);

  char mac_addr[6] = {0xDE, 0xAB, 0x12, 0x0A, 0xD1, 0xCF};
  int x = change_mac_addr( tap_fd, mac_addr);

  struct in_addr ipv4_addr;
  inet_aton("179.10.2.99", &ipv4_addr);
  struct in_addr netmask;
  inet_aton("255.255.255.0", &netmask);

  x = change_ipv4_addr( tap_name, &ipv4_addr, &netmask);
  if (x < 0) {
    printf("error code: %d\n", x);
  }
  else {
    char buffer[1500];
    while(1) {
    /* Note that "buffer" should be at least the MTU size of the interface, eg 1500 bytes */
      int bytes_read;
      if((bytes_read = read(tap_fd, buffer, sizeof(buffer))) < 0) {
        perror("Reading from interface");
        close(tap_fd);
        return -1;
      }
      
      printf("Read %d bytes from device %s\n", bytes_read, tap_name);

      struct packet packet;
      packet.pos = buffer;
      packet.remaining_length = bytes_read;
      
      char *x = advance_pos(&packet, sizeof(struct ethhdr));
      if(x == NULL) {
        printf("Not enough bytes for an ethernet header\n");
      }
      // Cast the 
      struct ethhdr *eth_header = (struct ethhdr *)x;
      eth_header->h_proto = ntohs(eth_header->h_proto);
      
      char dst_mac_addr[FORMATTED_MAC_ADDR_SIZE];
      char src_mac_addr[FORMATTED_MAC_ADDR_SIZE];
      format_mac_addr(eth_header->h_dest, dst_mac_addr);
      format_mac_addr(eth_header->h_source, src_mac_addr);
      printf("Destination: %s\nSource: %s\nType: %04x\n",
             dst_mac_addr, src_mac_addr, eth_header->h_proto);
      
      
    }
  }
  return 0;
}

