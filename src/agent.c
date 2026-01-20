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
#include <unistd.h>
#include <sys/wait.h>
#include "../common.h"
#include "ifproc.c"


void run_tcp_service() {
    int welcome_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    if ((welcome_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("TCP Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(welcome_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_TCP);

    if (bind(welcome_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(welcome_sock, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        client_sock = accept(welcome_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        if (fork() == 0) {
            close(welcome_sock);

            memset(buffer, 0, BUFFER_SIZE);
            int n = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);

            if (n > 0) {
                buffer[n] = '\0';

                if (strcmp(buffer, CMD_GET_ALL) == 0) {
                    // Mode : toutes les interfaces
                    send_interface_data(client_sock, NULL, 1);
                }
                else if (strncmp(buffer, CMD_GET_IF, strlen(CMD_GET_IF)) == 0) {
                    // Mode : interface spécifique (on extrait le nom après la commande)
                    char *if_name = buffer + strlen(CMD_GET_IF) + 1;
                    send_interface_data(client_sock, if_name, 0);
                }
            }

            close(client_sock);
            exit(0);
        }

        close(client_sock);
    }
}

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
            int hops = 1;
            char *ptr = strchr(buffer, ':');
            if (ptr) hops = atoi(ptr + 1);

            sendto(sock, hostname, strlen(hostname), 0, (struct sockaddr *)&client_addr, addr_len);

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
        printf("[UDP] Service de découverte actif sur le port %d\n", PORT_UDP);
        run_udp_service();
    } else {
        printf("[TCP] Service d'interrogation actif sur le port %d\n", PORT_TCP);
        run_tcp_service();
        wait(NULL);
    }

    return 0;
}