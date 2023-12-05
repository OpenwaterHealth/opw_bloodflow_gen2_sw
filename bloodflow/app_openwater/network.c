#include <stdio.h>
#include <string.h>
#include "network.h"

void getHostIpAddress(char *host_ipaddress)
{
    /* Get the IPaddress of the device for display */
    struct ifaddrs *ifaddr;
    int family;

    getifaddrs(&ifaddr);

    for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
         ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        /*
        printf("%-8s %s (%d)\n",
               ifa->ifa_name,
               (family == AF_PACKET) ? "AF_PACKET" : (family == AF_INET) ? "AF_INET"
                                                 : (family == AF_INET6)  ? "AF_INET6"
                                                                         : "???",
               family);
        */
        getnameinfo(ifa->ifa_addr,
                    (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                    host_ipaddress, NI_MAXHOST,
                    NULL, 0, NI_NUMERICHOST);
        //printf("\t\taddress: <%s>\n", host_ipaddress);

        if ((strcmp(ifa->ifa_name, "eth0") == 0) && (family == AF_INET))
            break;
    }
}