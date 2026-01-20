//
// Created by npas on 26/12/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "../common.h"

int main() {
  int sock;
  struct sockaddr_in broadcast_addr;
  char buffer[BUFFER_SIZE];
  int broadcast_permission = 1;

  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Socket UDP échouée");
    exit(1);
  }

  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0) {
    perror("Erreur setsockopt broadcast");
    exit(1);
  }

  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  broadcast_addr.sin_family = AF_INET;
  broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST; // 255.255.255.255
  broadcast_addr.sin_port = htons(PORT_UDP);

  printf("Recherche des machines voisines...\n");
  sendto(sock, DISCOVERY_PREFIX, strlen(DISCOVERY_PREFIX), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));

  struct sockaddr_in sender_addr;
  socklen_t addr_len = sizeof(sender_addr);

  printf("\nMachines trouvées :\n");
  printf("------------------\n");

  while (1) {
    int n = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&sender_addr, &addr_len);
    if (n < 0) break;

    buffer[n] = '\0';
    char *ip_sender = inet_ntoa(sender_addr.sin_addr);
    printf("%-20s (IP: %s)\n", buffer, ip_sender);
  }

  close(sock);
  return 0;
}