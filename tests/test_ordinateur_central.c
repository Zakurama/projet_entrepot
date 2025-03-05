#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "ordi_central.h"
#include "utils.h"

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

void test_parse_client_request_good_request(void) {
    const char *request = "item0_2,item1_1";
    int maxitems = 50;
    int L_n[maxitems];

    char *item_names[maxitems];
    for (int i = 0; i < maxitems; i++) {
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    int count;
    char *response = parse_client_request(request, maxitems, L_n, item_names, &count);

    // Validate results
    CU_ASSERT_PTR_NULL(response); // Check if there was no error
    CU_ASSERT_EQUAL(count, 2);
    CU_ASSERT_EQUAL(L_n[0], 2);
    CU_ASSERT_STRING_EQUAL(item_names[0], "item0");
    CU_ASSERT_EQUAL(L_n[1], 1);
    CU_ASSERT_STRING_EQUAL(item_names[1], "item1");

    // Cleanup memory
    for (int i = 0; i < maxitems; i++) {
        free(item_names[i]);
    }
}

void test_parse_client_request_invalid_format(void) {
    const char *request = "item0-2,item1_1";
    int maxitems = 50;
    int L_n[maxitems];

    char *item_names[maxitems];
    for (int i = 0; i < maxitems; i++) {
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    int count;
    char *response = parse_client_request(request, maxitems, L_n, item_names, &count);

    // Validate results
    CU_ASSERT_PTR_NOT_NULL(response); // Check if there was an error
    CU_ASSERT_STRING_EQUAL(response, "Invalid request format\n");

    // Cleanup memory
    for (int i = 0; i < maxitems; i++) {
        free(item_names[i]);
    }
}

void test_parse_client_request_too_many_items(void) {
    const char *request = "item0_2,item1_1,item2_3,item3_4,item4_5,item5_6,item6_7,item7_8,item8_9,item9_10,item10_11";
    int maxitems = 10;
    int L_n[maxitems];

    char *item_names[maxitems];
    for (int i = 0; i < maxitems; i++) {
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    int count;
    char *response = parse_client_request(request, maxitems, L_n, item_names, &count);

    // Validate results
    CU_ASSERT_PTR_NOT_NULL(response); // Check if there was an error
    CU_ASSERT_STRING_EQUAL(response, "Too many items requested\n");

    // Cleanup memory
    for (int i = 0; i < maxitems; i++) {
        free(item_names[i]);
    }
}

void test_parse_client_request_empty_request(void) {
    const char *request = "";
    int maxitems = 50;
    int L_n[maxitems];

    char *item_names[maxitems];
    for (int i = 0; i < maxitems; i++) {
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    int count;
    char *response = parse_client_request(request, maxitems, L_n, item_names, &count);

    // Validate results
    CU_ASSERT_PTR_NULL(response); // Check if there was no error
    CU_ASSERT_EQUAL(count, 0);

    // Cleanup memory
    for (int i = 0; i < maxitems; i++) {
        free(item_names[i]);
    }
}

void test_parse_stock_good_request(void) {
    const char *request = "item0;2_1.1,2_2.2/item1;1_3.3";
    int max_elements = 50;
    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    char *item_names[max_elements];
    int count[max_elements];
    int nb_items_request;

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    char *response = parse_stock(request, max_elements, L_n, L_x, L_y, item_names, count, &nb_items_request);

    // Validate results
    CU_ASSERT_PTR_NULL(response); // Check if there was no error
    CU_ASSERT_EQUAL(nb_items_request, 2);

    CU_ASSERT_EQUAL(count[0], 2);
    CU_ASSERT_STRING_EQUAL(item_names[0], "item0");
    CU_ASSERT_EQUAL(L_n[0][0], 2);
    CU_ASSERT_EQUAL(L_x[0][0], 0);
    CU_ASSERT_EQUAL(L_y[0][0], 0);
    CU_ASSERT_EQUAL(L_n[0][1], 2);
    CU_ASSERT_EQUAL(L_x[0][1], 1);
    CU_ASSERT_EQUAL(L_y[0][1], 1);

    CU_ASSERT_EQUAL(count[1], 1);
    CU_ASSERT_STRING_EQUAL(item_names[1], "item1");
    CU_ASSERT_EQUAL(L_n[1][0], 1);
    CU_ASSERT_EQUAL(L_x[1][0], 2);
    CU_ASSERT_EQUAL(L_y[1][0], 2);

    // Cleanup memory
    for (int i = 0; i < max_elements; i++) {
        free(L_n[i]);
        free(L_x[i]);
        free(L_y[i]);
        free(item_names[i]);
    }
}

void test_parse_stock_invalid_format(void) {
    const char *request = "item0;2_1.1,2_2.2/item1-1_3.3";
    int max_elements = 50;
    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    char *item_names[max_elements];
    int count[max_elements];
    int nb_items_request;

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    char *response = parse_stock(request, max_elements, L_n, L_x, L_y, item_names, count, &nb_items_request);

    // Validate results
    CU_ASSERT_PTR_NOT_NULL(response); // Check if there was an error
    CU_ASSERT_STRING_EQUAL(response, "Invalid request format\n");

    // Cleanup memory
    for (int i = 0; i < max_elements; i++) {
        free(L_n[i]);
        free(L_x[i]);
        free(L_y[i]);
        free(item_names[i]);
    }
}

void test_parse_stock_too_many_items(void) {
    const char *request = "item0;2_1.1,2_2.2/item1;1_3.3/item2;1_4.4/item3;1_5.5/item4;1_6.6/item5;1_7.7/item6;1_8.8/item7;1_9.9/item8;1_10.10/item9;1_11.11/item10;1_12.12";
    int max_elements = 10;
    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    char *item_names[max_elements];
    int count[max_elements];
    int nb_items_request;

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    char *response = parse_stock(request, max_elements, L_n, L_x, L_y, item_names, count, &nb_items_request);

    // Validate results
    CU_ASSERT_PTR_NOT_NULL(response); // Check if there was an error
    CU_ASSERT_STRING_EQUAL(response, "Too many items requested\n");

    // Cleanup memory
    for (int i = 0; i < max_elements; i++) {
        free(L_n[i]);
        free(L_x[i]);
        free(L_y[i]);
        free(item_names[i]);
    }
}

void test_parse_stock_empty_request(void) {
    const char *request = "";
    int max_elements = 50;
    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    char *item_names[max_elements];
    int count[max_elements];
    int nb_items_request;

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    char *response = parse_stock(request, max_elements, L_n, L_x, L_y, item_names, count, &nb_items_request);

    // Validate results
    CU_ASSERT_STRING_EQUAL(response, "Invalid request format\n");

    // Cleanup memory
    for (int i = 0; i < max_elements; i++) {
        free(L_n[i]);
        free(L_x[i]);
        free(L_y[i]);
        free(item_names[i]);
    }
}

int main() {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Tests Ordinateur Central", NULL, NULL);

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

    if (NULL == CU_add_test(suite, "test parse client good request", test_parse_client_request_good_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test parse client invalid format", test_parse_client_request_invalid_format)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test parse client too many items", test_parse_client_request_too_many_items)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test parse client empty request", test_parse_client_request_empty_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test parse stock good request", test_parse_stock_good_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test parse stock invalid format", test_parse_stock_invalid_format)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test parse stock too many items", test_parse_stock_too_many_items)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test parse stock empty request", test_parse_stock_empty_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}
