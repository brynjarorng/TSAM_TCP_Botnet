#include "ip.h"

// Short code to get own ip address from the assignment description with a few modifications
std::string get_ip()
{
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[64];

    if(getifaddrs(&myaddrs) != 0)
    {
        perror("getifaddrs");
        exit(1);
    }

    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;

        switch (ifa->ifa_addr->sa_family)
        {
            case AF_INET:
            {
                struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
                in_addr = &s4->sin_addr;
                break;
            }

            default:
                continue;
        }

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf)))
        {
            printf("%s: inet_ntop failed!\n", ifa->ifa_name);
        }
        else
        {
            // filter out loopback address
            if (std::string(buf) == "127.0.0.1")
                continue;
            //printf("%s: %s\n", ifa->ifa_name, buf);
            freeifaddrs(myaddrs);

            return std::string(buf);
        }
    }

    freeifaddrs(myaddrs);
    return std::string("127.0.0.1");
}
