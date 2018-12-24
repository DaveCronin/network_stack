
#ifndef UTILS_H
#define UTILS_H



// bytes requred to store a MAC address in ASCII
#define FORMATTED_MAC_ADDR_SIZE 18 

/*
  args: pointer to memory containing MAC address,
        buffer to place the output, must have 18 bytes allocated 
        (12 for MAC (each char is a byte) and 5 colons 
         + NULL terminator)
  
  This function formats a MAC addre into 12 hex characters divided 
  by colons.
  
*/

void format_mac_addr(char *data, char *formatted);


/*
  Struct for storing the current location of the parser in the message.
  This struct should only be used in conjunction with strict functions 
  that will update the remaining length when it changes the position.
 */

struct packet {
  char *pos;                       // current byte
  unsigned int remaining_length;   // remaining number of bytes 
};


/*
  args: The packet struct being modified.
        The number of bytes that we want to advance in the packet.

        The idea of this function is to confirm that there are enough
        remaining bytes in the packet to advance the amount specified,
        and then update the packet struct to reflect this advance
        if possible. 

        Returns the pointer to the original position so that the data
        at that part of the message can be processed, or NULL if there 
        is not enough data.
 */

char* advance_pos(struct packet *packet, int n_bytes);

#endif /* UTILS_H */
