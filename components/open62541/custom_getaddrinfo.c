

#include <lwip/netdb.h>
#include <esp_log.h>

int
custom_getaddrinfo(const char *nodename, const char *servname,
                 const struct addrinfo *hints, struct addrinfo **res)
{
    #ifdef CONFIG_ETHERNET_HELPER_CUSTOM_HOSTNAME
    if (strcmp(nodename, CONFIG_ETHERNET_HELPER_CUSTOM_HOSTNAME_STR) == 0) {
        // ESP32 does not correctly handle the case where getaddrinfo is called with its own hostname. Therefore just pass NULL in this case
        return lwip_getaddrinfo(NULL, servname, hints, res);
    }
    else {
        return lwip_getaddrinfo(nodename, servname, hints, res);
    }
    #else
    return lwip_getaddrinfo(NULL, servname, hints, res);
    #endif
}