#include <sys/ioctl.h> // ioctl
#include <sys/socket.h> // socket
#include <netinet/in.h> // sockaddr_in, in_adr
#include <fcntl.h> // for open
#include <unistd.h> // for close, read

#include <arpa/inet.h> // hton, ntoh

#include "common.h"
#include "tuntap.h"
#include "utils.h"


int tuntap_alloc(char *dev, int flags) {

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


