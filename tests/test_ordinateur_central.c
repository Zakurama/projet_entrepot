#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "ordi_central.h"

void test_trajectoire_generique(const char *pos_initiale, const char *pos_finale, char expected[][SIZE_POS]) {
    char path[MAX_WAYPOINTS][SIZE_POS];

    // Initialisation des éléments du tableau
    for (int i = 0; i < MAX_WAYPOINTS; i++) {
        path[i][0] = '\0';  // Mettre une chaîne vide
    }

    trajectoire(pos_initiale, pos_finale, path);

    for (int i = 0; i < MAX_WAYPOINTS; i++) {
        if (expected[i][0] == '\0' && path[i][0] == '\0') {
            break;
        }
        CU_ASSERT_STRING_EQUAL(path[i], expected[i]);
    }
}

void test_trajectoire_D25_S14() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "D25", "M25", "M20", "M15", "M10","S11","S12","S13","S14" 
    };
    test_trajectoire_generique("D25", "S14", expected);
}
void test_trajectoire_S14_S11() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "S14", "S13", "S12", "S11"
    };
    test_trajectoire_generique("S14", "S11", expected);
}

void test_trajectoire_P25_S44() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "P25", "D25", "D30", "D35", "D40", "M40", "S41", "S42", "S43", "S44"
    };
    test_trajectoire_generique("P25", "S44", expected);
}

void test_trajectoire_S44_S11() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "S44", "S43", "S42", "S41", "S40", "M40", "M35", "M30", "M25", "M20", "M15", "M10", "S11"
    };
    test_trajectoire_generique("S44", "S11", expected);
}

void test_trajectoire_S11_S44() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "S11", "S10", "M10", "D10", "D15", "D20", "D25", "D30", "D35", "D40", "M40", "S41", "S42", "S43", "S44"
    };
    test_trajectoire_generique("S11", "S44", expected);
}

void test_trajectoire_S14_P25() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "S14", "S13", "S12", "S11", "S10", "M10", "D10", "D15", "D20", "D25", "P25"
    };
    test_trajectoire_generique("S14", "P25", expected);
}

void test_trajectoire_P25_B5() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "P25", "D25", "M25", "M20", "M15", "M10", "M5", "D5", "B5"
    };
    test_trajectoire_generique("P25", "B5", expected);
}

void test_trajectoire_B5_P25() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "B5", "B10", "P25"
    };
    test_trajectoire_generique("B5", "P25", expected);
}

void test_trajectoire_B10_P25() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "B10", "P25"
    };
    test_trajectoire_generique("B10", "P25", expected);
}

void test_trajectoire_D30_B10() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "D30", "M30", "M25", "M20", "M15", "M10", "D10", "B10"
    };
    test_trajectoire_generique("D30", "B10", expected);
}
int main() {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Tests Trajectoire", NULL, NULL);

    CU_add_test(suite, "Trajectoire de D25 à S14", test_trajectoire_D25_S14);
    CU_add_test(suite, "Trajectoire de S14 à S11", test_trajectoire_S14_S11);
    CU_add_test(suite, "Trajectoire de P25 à S44", test_trajectoire_P25_S44);
    CU_add_test(suite, "Trajectoire de S44 à S11", test_trajectoire_S44_S11);
    CU_add_test(suite, "Trajectoire de S11 à S44", test_trajectoire_S11_S44);
    CU_add_test(suite, "Trajectoire de S14 à P25", test_trajectoire_S14_P25);
    CU_add_test(suite, "Trajectoire de P25 à S11", test_trajectoire_P25_B5);
    CU_add_test(suite, "Trajectoire de B5 à P25", test_trajectoire_B5_P25);
    CU_add_test(suite, "Trajectoire de B10 à P25", test_trajectoire_B10_P25);
    CU_add_test(suite, "Trajectoire de D30 à B10", test_trajectoire_D30_B10);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}