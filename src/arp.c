#include <stdint.h>
#include <netinet/in.h> // sockaddr_in, in_adr


#include "common.h"
#include "utils.h"
#include "arp.h"

void print_arp_message(struct arp_hdr *arp_header,
                       struct packet *sender_hw_addr,
                       struct packet *sender_proto_addr,
                       struct packet *target_hw_addr,
                       struct packet *target_proto_addr) {
  printf("%04x, %04x, %02x, %02x, %04x\n",
         arp_header->hw_proto,
         arp_header->req_proto,
         arp_header->hw_size,
         arp_header->req_size,
         arp_header->opcode);

  char sender_hw_addr_buf[FORMATTED_MAC_ADDR_SIZE];
  format_mac_addr(sender_hw_addr->pos, sender_hw_addr_buf);
  char target_hw_addr_buf[FORMATTED_MAC_ADDR_SIZE];
  format_mac_addr(target_hw_addr->pos, target_hw_addr_buf);
   
  if (arp_header->opcode == 1){
    printf("Recieved ARP request, who has %s tell %s\n",
           inet_ntoa(*(struct in_addr*)target_proto_addr->pos),
           sender_hw_addr_buf);
    printf("Other: %s, %s\n",
         inet_ntoa(*(struct in_addr*)sender_proto_addr->pos),
         target_hw_addr_buf);
  }
}
                       

int parse_arp_message(struct packet *packet) {
  // Check if there is enough data for an ethernet header.
  struct arp_hdr *arp_header =
    (struct arp_hdr *)advance_pos(packet, sizeof(struct arp_hdr));
  if(arp_header == NULL) {
    printf("Not enough bytes for an ARP header\n");
    return -1;
  }

  // The potocol must be converted to host order (2 octets)
  //  arp_header->h_proto = ntohs(arp_header->h_proto);
  arp_header->hw_proto = ntohs(arp_header->hw_proto);
  arp_header->req_proto = ntohs(arp_header->req_proto);
  arp_header->opcode = ntohs(arp_header->opcode);

  if (packet->remaining_length <
      (arp_header->hw_size+arp_header->req_size)*2) {
    printf("Not enough bytes for the advertised ARP payload\n");
    return -1;
  }

  // safe to proceed parsing
  struct packet sender_hw_addr;
  sender_hw_addr.pos = advance_pos(packet, arp_header->hw_size);
  sender_hw_addr.remaining_length=arp_header->hw_size;

  struct packet sender_proto_addr;
  sender_proto_addr.pos = advance_pos(packet, arp_header->req_size);
  sender_proto_addr.remaining_length=arp_header->req_size;

  struct packet target_hw_addr;
  target_hw_addr.pos = advance_pos(packet, arp_header->hw_size);
  target_hw_addr.remaining_length=arp_header->hw_size;
  
  struct packet target_proto_addr;
  target_proto_addr.pos = advance_pos(packet, arp_header->req_size);
  target_proto_addr.remaining_length=arp_header->req_size;

  print_arp_message(arp_header, &sender_hw_addr, &sender_proto_addr,
                    &target_hw_addr, &target_proto_addr);
  
  return 0;
}
