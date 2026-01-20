//
// Created by npas on 19/11/2026.
//

#define POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>


int calculer_prefixe(int famille, struct sockaddr *masque) {
    if (masque == NULL) return (famille == AF_INET) ? 32 : 128;
    unsigned char *bytes;
    int taille_bytes, prefixe = 0;

    if (famille == AF_INET) {
        bytes = (unsigned char *)&((struct sockaddr_in *)masque)->sin_addr;
        taille_bytes = 4;
    } else {
        bytes = (unsigned char *)&((struct sockaddr_in6 *)masque)->sin6_addr;
        taille_bytes = 16;
    }

    for (int i = 0; i < taille_bytes; i++) {
        unsigned char b = bytes[i];
        while (b > 0) { if (b & 1) prefixe++; b >>= 1; }
    }
    return prefixe;
}

void afficher_interface_fd(int fd, struct ifaddrs *ifa) {
    int famille = ifa->ifa_addr->sa_family;
    char host[1025];

    if (famille != AF_INET && famille != AF_INET6) return;

    int s = getnameinfo(ifa->ifa_addr,
                        (famille == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                        host, 1025, NULL, 0, NI_NUMERICHOST);

    if (s != 0) return;

    int cidr = calculer_prefixe(famille, ifa->ifa_netmask);

    dprintf(fd, "%s : %s/%d (%s)\n",
           ifa->ifa_name, host, cidr, (famille == AF_INET) ? "IPv4" : "IPv6");
}

void send_interface_data(int fd, const char *target_if, int mode_all) {
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        dprintf(fd, "Erreur interne: getifaddrs a échoué.\n");
        return;
    }

    int found = 0;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if (mode_all) {
            afficher_interface_fd(fd, ifa);
            found = 1;
        } else if (target_if != NULL && strcmp(ifa->ifa_name, target_if) == 0) {
            afficher_interface_fd(fd, ifa);
            found = 1;
        }
    }

    if (!found && target_if != NULL) {
        dprintf(fd, "Interface '%s' introuvable ou sans adresse IP.\n", target_if);
    }
    freeifaddrs(ifaddr);
}