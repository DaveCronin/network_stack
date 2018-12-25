#ifndef ARP_H
#define ARP_H

#include <linux/types.h>

struct arp_hdr {
  uint16_t	hw_proto; 
  uint16_t	req_proto;
  unsigned char	hw_size;
  unsigned char	req_size;
  uint16_t      opcode;

} __attribute__((packed));


struct addr {
  char *addr;     // current byte
  uint8_t size;   // remaining number of bytes
  char proto;     // protocol
  
};

int parse_arp_message(struct packet *packet);

#endif /* ARP_H */
