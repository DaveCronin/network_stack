#include <unistd.h> // for close, read

#include "common.h"
#include "tuntap.h"
#include "utils.h"
#include "ethernet.h"

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
  if (tap_fd < 0) {
    return -1;
  }

  char mac_addr[MAC_ADDR_SIZE] = {0xDE, 0xAB, 0x12, 0x0A, 0xD1, 0xCF};

  if (change_mac_addr( tap_fd, mac_addr)) {
    return -1;
  }


  struct in_addr ipv4_addr;
  inet_aton("179.10.2.99", &ipv4_addr);
  struct in_addr netmask;
  inet_aton("255.255.255.0", &netmask);

  if (change_ipv4_addr( tap_name, &ipv4_addr, &netmask)) {
    return -1;
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
      
      printf("\nRead %d bytes from device %s\n", bytes_read, tap_name);

      struct packet packet;
      packet.pos = buffer;
      packet.remaining_length = bytes_read;

      int x = parse_ethernet_frame(tap_fd, &packet);
      assert(packet.remaining_length == 0 && "Trailing bytes..\n");
    }
  }
  return 0;
}

