#ifndef TUNTAP_H
#define TUNTAP_H

/*
  Included here as it will be needed for setting 
  flags when calling these functions.
*/
#include <linux/if.h> // ifreq etc
#include <linux/if_tun.h> // TUNSETIFF etc, 

#define CLONE_DEVICE "/dev/net/tun"
#define ETHER_FAMILY 1

/* 
   args: pointer to buffer with device name,
         flags to indicate the type of interface 
         (tun or tap: IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI)

   dev can be a null to let the kernel allocate the next device of
   that type, eg tap(X+1).
   If successful, the dev buffer will allocated device name writen 
   to it for the caller. This means that the caller must reserve space
   of size IFNAMSIZ to have space to store the device name.

   Returns the fd for teh virtual interface on success 
   and -1 on failure.
*/

int tuntap_alloc(char *dev, int flags);


/*
  args: file descriptor for interface, 
        buffer containing mac addr in first 6 bytes
  
  Only the first 6 bytes of the buffer will be used to set the
  MAC address on the interface provided by the fd.
  
  Returns 0 on success and -1 on failure.
*/

int change_mac_addr(int fd, char* addr);



/*
  args: pointer to buffer with device name,
        addr points to a struct containing the desired IPv4 address
        netmask points to a struct containing the desired netmask
        (prefix length)
  
  This function will set the IPv4 address and netmask on the 
  interface with a provided name.

  Returns 0 on success and -1 on failure.
*/

int change_ipv4_addr(char *dev, struct in_addr *addr,
                     struct in_addr *netmask);



#endif /* TUNTAP_H */
