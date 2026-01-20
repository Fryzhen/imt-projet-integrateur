//
// Created by npas on 03/11/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ifproc.c"

void usage(const char *prog_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s -a            : Affiche toutes les interfaces\n", prog_name);
    fprintf(stderr, "  %s -i <ifname>   : Affiche l'interface spécifiée\n", prog_name);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    char *target_if = NULL;
    int mode_all = 0;

    if (argc < 2) usage(argv[0]);

    if (strcmp(argv[1], "-a") == 0) {
        mode_all = 1;
    } else if (strcmp(argv[1], "-i") == 0) {
        if (argc < 3) usage(argv[0]);
        target_if = argv[2];
    } else {
        usage(argv[0]);
    }

    send_interface_data(STDOUT_FILENO, target_if, mode_all);

    return 0;
}