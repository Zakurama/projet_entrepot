#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "ordi_central.h"

void test_trajectoire(const char *pos_initiale, const char *pos_finale) {
    char path[MAX_WAYPOINTS][SIZE_POS];

    // Initialisation des éléments du tableau
    for (int i = 0; i < MAX_WAYPOINTS; i++) {
        path[i][0] = '\0';  // Mettre une chaîne vide
    }

    trajectoire(pos_initiale, pos_finale, path);

    for (int i = 0; i < MAX_WAYPOINTS; i++) {
        if (strcmp(path[i], "\0") != 0) {
            if (i == 0) {
                printf("%s", path[i]);
            } else {
                printf(" -> %s", path[i]);
            }
        } else {
            printf("\n");
            break;
        }
    }
}

int main() {
    printf("Trajectoire de D25 à S14\n");
    test_trajectoire("D25", "S14");
    
    printf("Trajectoire de S14 à S11\n");
    test_trajectoire("S14", "S11");

    printf("Trajectoire de P25 à S44\n");
    test_trajectoire("P25", "S44");

    printf("Trajectoire de S44 à S11\n");
    test_trajectoire("S44", "S11");

    printf("Trajectoire de S11 à S44\n");
    test_trajectoire("S11", "S44");

    printf("Trajectoire de S14 à D40\n");
    test_trajectoire("S14", "D40");
    return 0;
}