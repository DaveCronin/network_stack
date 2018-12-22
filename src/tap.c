#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <errno.h>

#define CLONE_DEVICE "/dev/net/tun"
#define ETHER_FAMILY 1

/*
  args: pointer to memory containing MAC address,
        buffer to place the output, must have 18 bytes allocated 
        (12 for MAC (each char is a byte) and 5 colons 
         + NULL terminator)
  
  This function formats a MAC addre into 12 hex characters divided 
  by colons.
  
 */
#define FORMATTED_MAC_ADDR_SIZE 18
void format_mac_addr(char *data, char *formatted) {
  sprintf(formatted, "%02X:%02X:%02X:%02X:%02X:%02X\0",
         (unsigned char) data[0],
         (unsigned char) data[1],
         (unsigned char) data[2],
         (unsigned char) data[3],
         (unsigned char) data[4],
         (unsigned char) data[5]);
}

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

int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;

  memset(&ifr, 0, sizeof(ifr));
  
  // open the clone device 
  if( (fd = open(CLONE_DEVICE, O_RDWR)) < 0 ) {
    printf( "open %s: %s\n" CLONE_DEVICE, strerror(errno) );
    return -1;
  }

  ifr.ifr_flags = flags;

  if (*dev) {
    // if specified fill out struct with name.
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    printf( "Requested to create %.*s\n", IFNAMSIZ, dev);
  }

  // try to create the device 
  if( ioctl(fd, TUNSETIFF, &ifr) < 0 ) {
    printf( "TUNSETIFF: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  // Write back the device name.
  strcpy(dev, ifr.ifr_name);
  printf( "Created %.*s\n", IFNAMSIZ, dev);
  
  return fd;
}

/*
  args: file descriptor for interface, 
        buffer containing mac addr in first 6 bytes
  
  Only the first 6 bytes of the buffer will be used to set the
  MAC address on the interface provided by the fd.
  
  Returns 0 on success and -1 on failure.
*/

int change_mac_addr(int fd, char* addr) {
  struct ifreq ifr_orig;
  struct ifreq ifr;
  char mac_buffer_orig[FORMATTED_MAC_ADDR_SIZE];
  char mac_buffer[FORMATTED_MAC_ADDR_SIZE];
  
  memset(&ifr_orig, 0, sizeof(ifr_orig));
  memset(&ifr, 0, sizeof(ifr));
  
  // Get the existing MAC address
  if( ioctl(fd, SIOCGIFHWADDR, &ifr_orig) < 0 ) {
    printf( "SIOCGIFHWADDR: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  // Copy the new MAC address to the struct
  strncpy(ifr.ifr_hwaddr.sa_data, addr, 6);
  ifr.ifr_hwaddr.sa_family = ETHER_FAMILY;

  // Set the new MAC address
  if( ioctl(fd, SIOCSIFHWADDR, &ifr) < 0 ) {
    printf( "SIOCSIFHWADDR: %s\n", strerror(errno));
    close(fd);
    return -1;
  }

  // Format the original and new MAC addresses for printing
  format_mac_addr(ifr_orig.ifr_hwaddr.sa_data, mac_buffer_orig);
  format_mac_addr(ifr.ifr_hwaddr.sa_data, mac_buffer);
  printf( "Changed MAC address from %s to %s\n", mac_buffer_orig,
          mac_buffer);

  return 0;
}

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
                     struct in_addr *netmask) {
  struct ifreq ifr;
  struct sockaddr_in new_addr;
  int soc;
 
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  // create socket, for some reason this is required to set
  // ip information on the device.
  if ( (soc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    printf( "socket: %s\n", strerror(errno));
    return -1;
  }
  
  memset(&new_addr, 0, sizeof(new_addr));
  new_addr.sin_family = AF_INET;
  memcpy(&new_addr.sin_addr, addr, sizeof(addr));
  memcpy(&ifr.ifr_addr, &new_addr, sizeof(ifr.ifr_addr));
  
  // Set the new IPv4 address
  if( ioctl(soc, SIOCSIFADDR, &ifr) < 0 ) {
    printf( "SIOCSIFHWADDR: %s\n", strerror(errno));
    close(soc);
    return -1;
  }

  memset(&new_addr, 0, sizeof(new_addr));
  new_addr.sin_family = AF_INET;
  memcpy(&new_addr.sin_addr, netmask, sizeof(netmask));
  memcpy(&ifr.ifr_netmask, &new_addr, sizeof(ifr.ifr_netmask));
  
  // Set the new IPv4 netmask
  if( ioctl(soc, SIOCSIFNETMASK, &ifr) < 0 ) {
    printf( "SIOCSIFNETMASK: %s\n", strerror(errno));
    close(soc);
    return -1;
  }

  // The two inet_ntoa calls must be in seperate printf calls
  // since inet_toa returns a pointer to a static buffer (which
  // persists between calls) so the second call would overwrite the
  // first.
  printf( "Changed IPv4 address on %s to %s ", dev, inet_ntoa(*addr));
  printf( "with netmask %s\n", inet_ntoa(*netmask));
  close(soc);
  return 0;
}

int main() {
  int tap_fd;
  char tap_name[IFNAMSIZ];
  strcpy(tap_name, "tap44");
  // TAP interface flags with no protocol information header
  // The header is 4 bytes [2:2, flags:protocol]
  // I couldn't find any good info on what this is so
  // I am removing the excess bytes and will assume
  // ethernet
  tap_fd = tun_alloc(tap_name, IFF_TAP | IFF_NO_PI);

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
    getchar();
    char buffer[1500];
    while(1) {
    /* Note that "buffer" should be at least the MTU size of the interface, eg 1500 bytes */
      int bytes_read;
      if((bytes_read = read(tap_fd, buffer, sizeof(buffer))) < 0) {
        perror("Reading from interface");
        close(tap_fd);
        return -1;
      }

      /* Do whatever with the data */
      printf("Read %d bytes from device %s\n", bytes_read, tap_name);
      //      printf("%02X:%02X:%02X:%02X", buffer[0], buffer[1], buffer[2], buffer[3]);
    }
  }
  return 0;
}

