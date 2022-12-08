#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>


#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>


// int main()
// {
//     unsigned char ip_address[15];
//     int fd;
//     struct ifreq ifr;

//     /*AF_INET - to define network interface IPv4*/
//     /*Creating soket for it.*/
//     fd = socket(AF_INET, SOCK_DGRAM, 0);

//     /*AF_INET - to define IPv4 Address type.*/
//     ifr.ifr_addr.sa_family = AF_INET;

//     /*eth0 - define the ifr_name - port name
//     where network attached.*/
//     memcpy(ifr.ifr_name, "docker0", IFNAMSIZ - 1);

//     /*Accessing network interface information by
//     passing address using ioctl.*/
//     ioctl(fd, SIOCGIFADDR, &ifr);
//     /*closing fd*/
//     close(fd);

//     /*Extract IP Address*/
//     strcpy(ip_address, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

//     printf("System IP Address is: %s\n", ip_address);

//     return 0;
// }

int main() {
    struct ifaddrs *addrs,*tmp;

    char ip_address[15];
    int fd;
    struct ifreq ifr;

    /*AF_INET - to define network interface IPv4*/
    /*Creating soket for it.*/
    // fd = socket(AF_INET, SOCK_DGRAM, 0);

    /*AF_INET - to define IPv4 Address type.*/
    ifr.ifr_addr.sa_family = AF_INET;

    /*Extract IP Address*/


    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET){
            /*eth0 - define the ifr_name - port name
            where network attached.*/
            memcpy(ifr.ifr_name, tmp->ifa_name, IFNAMSIZ - 1);
            
            /*Accessing network interface information by
            passing address using ioctl.*/
            //ioctl(0, SIOCGIFADDR, &ifr);
            /*closing fd*/
            strcpy(ip_address, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
            printf("%s %s\n", tmp->ifa_name,ip_address);

        }
        tmp = tmp->ifa_next;
    }

    // close(fd);
    freeifaddrs(addrs);
}
