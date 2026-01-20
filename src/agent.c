//
// Created by npas on 19/11/2025.
//

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../common.h"
#include "ifproc.c"

// --- SERVICE UDP : Découverte de voisinage ---
void run_udp_service() {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    char hostname[256];
    socklen_t addr_len = sizeof(client_addr);

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_UDP);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP Bind failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (n < 0) continue;
        buffer[n] = '\0';

        if (strcmp(buffer, DISCOVERY_PREFIX) == 0) {
            gethostname(hostname, sizeof(hostname));
            sendto(sock, hostname, strlen(hostname), 0, (struct sockaddr *)&client_addr, addr_len);
        }
    }
}

// --- SERVICE TCP : Requêtes d'interfaces (ifnetshow) ---
// À ajouter dans agent.c pour remplacer l'ancienne fonction UDP

void run_udp_service() {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    char hostname[256];
    socklen_t addr_len = sizeof(client_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)); // Permettre le re-broadcast

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_UDP);
    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    gethostname(hostname, sizeof(hostname));

    while (1) {
        int n_bytes = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (n_bytes < 0) continue;
        buffer[n_bytes] = '\0';

        if (strncmp(buffer, DISCOVERY_PREFIX, strlen(DISCOVERY_PREFIX)) == 0) {
            // 1. Extraire le nombre de hops
            int hops = 1;
            char *ptr = strchr(buffer, ':');
            if (ptr) hops = atoi(ptr + 1);

            // 2. Répondre à l'émetteur original
            // Note: Pour le multi-hop, il faudrait idéalement passer l'IP du client original
            // dans le message, mais pour simplifier ici, on répond à celui qui nous a parlé.
            sendto(sock, hostname, strlen(hostname), 0, (struct sockaddr *)&client_addr, addr_len);

            // 3. Propagation si hops > 1
            if (hops > 1) {
                char new_msg[64];
                snprintf(new_msg, sizeof(new_msg), "%s:%d", DISCOVERY_PREFIX, hops - 1);

                struct sockaddr_in brd_addr;
                brd_addr.sin_family = AF_INET;
                brd_addr.sin_port = htons(PORT_UDP);
                brd_addr.sin_addr.s_addr = INADDR_BROADCAST;

                // On attend un peu pour éviter les tempêtes de broadcast
                usleep(100000);
                sendto(sock, new_msg, strlen(new_msg), 0, (struct sockaddr *)&brd_addr, sizeof(brd_addr));
            }
        }
    }
}

int main() {
    printf("Démarrage de l'agent (VyOS/Alpine/MicroCore)...\n");

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // --- PROCESSUS ENFANT : Service de découverte (UDP) ---
        printf("[UDP] Service de découverte actif sur le port %d\n", PORT_UDP);
        run_udp_service();
    } else {
        // --- PROCESSUS PARENT : Service d'interrogation (TCP) ---
        printf("[TCP] Service d'interrogation actif sur le port %d\n", PORT_TCP);
        run_tcp_service();

        // Attendre l'enfant si nécessaire (théoriquement jamais ici car boucle infinie)
        wait(NULL);
    }

    return 0;
}