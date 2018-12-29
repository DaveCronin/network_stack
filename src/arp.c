#include <stdint.h>
#include <netinet/in.h> // sockaddr_in, in_adr
#include <linux/if_ether.h> // ETH_P_IP

#include "common.h"
#include "utils.h"
#include "arp.h"
#include "ethernet.h"

struct arp_table_entry {
  uint16_t      req_proto;
  char          addr[10];
  uint16_t      hw_proto;
  char          hw_addr[10];
  struct arp_table_entry *next;
};

static struct arp_table_entry *arp_table = NULL;

void print_arp_table( struct arp_table_entry *entry) {
  while(entry != NULL) {
    char hw_addr_buf[FORMATTED_MAC_ADDR_SIZE];
    format_mac_addr(entry->hw_addr, hw_addr_buf);
    printf("%s can be reached via %s\n",
           inet_ntoa(*(struct in_addr*)(entry->addr)),
           hw_addr_buf);
    entry = entry->next;
  }
}

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
   
  if (arp_header->opcode == ARP_REQUEST){
    printf("Recieved ARP request, who has %s tell %s\n",
           inet_ntoa(*(struct in_addr*)target_proto_addr->pos),
           sender_hw_addr_buf);
    printf("Other: %s, %s\n",
         inet_ntoa(*(struct in_addr*)sender_proto_addr->pos),
         target_hw_addr_buf);
  }
  else {
    printf("Recieved ARP reply, %s has %s\n",
           sender_hw_addr_buf,
           inet_ntoa(*(struct in_addr*)sender_proto_addr->pos));
    printf("Other: %s, %s\n",
         inet_ntoa(*(struct in_addr*)target_proto_addr->pos),
         target_hw_addr_buf);
  }
}

int send_arp_reply(struct arp_hdr *arp_header,
                  struct packet *sender_hw_addr,
                  struct packet *sender_proto_addr,
                  struct packet *target_hw_addr,
                  struct packet *target_proto_addr) {
  arp_header->opcode = ARP_REPLY;
  arp_header->hw_proto = htons(arp_header->hw_proto);
  arp_header->req_proto = htons(arp_header->req_proto);
  arp_header->opcode = htons(arp_header->opcode);

  char *message = calloc(sizeof(struct arp_hdr) +
                         2*(arp_header->hw_size +
                            arp_header->req_size), sizeof(char));
  memcpy(message, arp_header, sizeof(struct arp_hdr));
  
  char *curr_pos = message+sizeof(struct arp_hdr);

  char mac_addr[MAC_ADDR_SIZE] = {0xAB, 0xAB, 0xAB, 0xAB, 0xAB, 0xAB};
  struct in_addr my_addr;
  inet_aton("179.10.2.90", &my_addr);
  
  memcpy(curr_pos, mac_addr, arp_header->hw_size);
  curr_pos = curr_pos+arp_header->hw_size;
  
  memcpy(curr_pos, &my_addr.s_addr, arp_header->req_size);
  curr_pos = curr_pos+arp_header->req_size;
  memcpy(curr_pos, sender_hw_addr->pos, arp_header->hw_size);
  curr_pos = curr_pos+arp_header->hw_size;
  memcpy(curr_pos, sender_proto_addr->pos, arp_header->req_size);
  
  send_ethernet_frame(message, sizeof(struct arp_hdr) +
                         2*(arp_header->hw_size +
                            arp_header->req_size));
}

int handle_arp_message(struct arp_hdr *arp_header,
                       struct packet *sender_hw_addr,
                       struct packet *sender_proto_addr,
                       struct packet *target_hw_addr,
                       struct packet *target_proto_addr) {
  // Check if we support the hw_proto and verify it's length
  if(arp_header->hw_proto!=ETHER_FAMILY) {
    printf("Unsupported hw_proto %04x\n", arp_header->hw_proto);
    return -1;
  }
  if(arp_header->hw_size!=MAC_ADDR_SIZE) {
    printf("hw_size mismatch %02x should be %02x\n", arp_header->hw_size,
           MAC_ADDR_SIZE);
    return -1;
  }
      
  // Check if we support the req_proto (verify length)
  if(arp_header->req_proto!=ETH_P_IP) {
    printf("Unsupported req_proto %04x\n", arp_header->req_proto);
    return -1;
  }
  if(arp_header->req_size!=IPV4_ADDR_SIZE) {
    printf("req_size mismatch %02x should be %02x\n", arp_header->req_size,
           IPV4_ADDR_SIZE);
    return -1;
  }

  bool merge_flag=false;

  // Check if req_proto and sender_proto_addr are in translation table

  struct arp_table_entry *entry = arp_table;
  
  while(entry != NULL) {
    if (entry->req_proto == arp_header->req_proto &&
        strncmp(entry->addr, sender_proto_addr->pos, 4) == 0) {
      // Update the found entry and set merge_falg=true
      entry->hw_proto=arp_header->hw_proto;
      memcpy(entry->hw_addr, sender_hw_addr->pos, 6);
      merge_flag=true;
      break;
    }
    entry = entry->next;
  }

  struct in_addr my_addr;
  inet_aton("179.10.2.90", &my_addr);

  // check if we're the target
  if (((struct in_addr*)target_proto_addr->pos)->s_addr!=my_addr.s_addr) {
    printf("%s is not our address\n",
           inet_ntoa(*(struct in_addr*)target_proto_addr->pos));
    return -1;
  }
  
  struct arp_table_entry **curr_entry = &arp_table;
  if(!merge_flag) {
    if(*curr_entry != NULL) {
      while((*curr_entry)->next != NULL) {
        curr_entry = &(entry->next);
      }
    }
    struct arp_table_entry * new_entry = (struct arp_table_entry*) malloc(sizeof(struct arp_table_entry));
    new_entry->req_proto = arp_header->req_proto;
    memcpy(new_entry->addr, sender_proto_addr->pos, 4);
    new_entry->hw_proto = arp_header->hw_proto;
    memcpy(new_entry->hw_addr, sender_hw_addr->pos, 6);
    *curr_entry = new_entry;
  }

  if (arp_header->opcode == ARP_REQUEST){
    printf("We should send a reply\n");
    send_arp_reply(arp_header, sender_hw_addr, sender_proto_addr,
                       target_hw_addr, target_proto_addr);
    
    
  }
  printf("printing table\n");
  print_arp_table(arp_table);
  
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

  handle_arp_message(arp_header, &sender_hw_addr, &sender_proto_addr,
                    &target_hw_addr, &target_proto_addr);
  
  return 0;
}
