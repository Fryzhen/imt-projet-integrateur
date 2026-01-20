//
// Created by npas on 19/11/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common.h"

void usage(char *prog) {
  fprintf(stderr, "Usage: %s -n <addr> [-a | -i <ifname>]\n", prog);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  int sock = 0;
  struct sockaddr_in serv_addr;
  char *target_ip = NULL;
  char *target_if = NULL;
  int mode_all = 0;
  char buffer[BUFFER_SIZE];

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
      target_ip = argv[++i];
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      target_if = argv[++i];
    } else if (strcmp(argv[i], "-a") == 0) {
      mode_all = 1;
    }
  }

  if (target_ip == NULL || (!mode_all && target_if == NULL)) {
    usage(argv[0]);
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket creation error");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT_TCP);

  if (inet_pton(AF_INET, target_ip, &serv_addr.sin_addr) <= 0) {
    fprintf(stderr, "Adresse IP invalide : %s\n", target_ip);
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Connexion échouée (l'agent est-il lancé ?)");
    return -1;
  }

  if (mode_all) {
    snprintf(buffer, BUFFER_SIZE, "%s", CMD_GET_ALL);
  } else {
    snprintf(buffer, BUFFER_SIZE, "%s %s", CMD_GET_IF, target_if);
  }
  send(sock, buffer, strlen(buffer), 0);

  int valread;
  while ((valread = read(sock, buffer, BUFFER_SIZE - 1)) > 0) {
    buffer[valread] = '\0';
    printf("%s", buffer);
  }

  close(sock);
  return 0;
}