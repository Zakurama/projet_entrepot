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
        "D25","D30","D35","D40","M40","M35","M30", "M25", "M20", "M15", "M10","S11","S12","S13","S14" 
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
        "S11", "S10", "M10","M5","D5", "D10", "D15", "D20", "D25", "D30", "D35", "D40", "M40", "S41", "S42", "S43", "S44"
    };
    test_trajectoire_generique("S11", "S44", expected);
}

void test_trajectoire_S14_P25() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "S14", "S13", "S12", "S11", "S10", "M10","M5","D5", "D10", "D15", "D20", "D25", "P25"
    };
    test_trajectoire_generique("S14", "P25", expected);
}

void test_trajectoire_P25_B5() {
    char expected[MAX_WAYPOINTS][SIZE_POS] = {
        "P25", "D25","D30","D35","D40","M40","M35","M30", "M25", "M20", "M15", "M10", "M5", "D5", "B5"
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
        "D30","D35","D40","M40","M35", "M30", "M25", "M20", "M15", "M10", "D10", "B10"
    };
    test_trajectoire_generique("D30", "B10", expected);
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

void test_selection_items(void){
    // Liste des articles demandés par le client
    char *item_names_requested[] = {"Banane", "Pomme", "Raisin"};
    int L_n_requested[] = {6, 5, 8}; // Quantités demandées
    int count_requested = 3;

    // Stock disponible
    char *item_names_stock[] = {"Banane", "Pomme", "Orange", "Raisin"};
    int L_n_stock_0[] = {5, 6};  // Stock de "Banane" à deux positions
    int L_n_stock_1[] = {3, 2};  // Stock de "Pomme" à deux positions
    int L_n_stock_2[] = {4, 7};  // Stock de "Orange" (non demandé)
    int L_n_stock_3[] = {2, 6};  // Stock de "Raisin" à deux positions

    // Pointeurs vers les stocks
    int *L_n_stock[] = {L_n_stock_0, L_n_stock_1, L_n_stock_2, L_n_stock_3};

    // Position des articles dans l'entrepôt (allées et bacs)
    int L_x_stock_0[] = {1, 2};
    int L_x_stock_1[] = {3, 4};
    int L_x_stock_2[] = {5, 6};
    int L_x_stock_3[] = {7, 8};
    int *L_x_stock[] = {L_x_stock_0, L_x_stock_1, L_x_stock_2, L_x_stock_3};

    int L_y_stock_0[] = {10, 11};
    int L_y_stock_1[] = {12, 13};
    int L_y_stock_2[] = {14, 15};
    int L_y_stock_3[] = {16, 17};
    int *L_y_stock[] = {L_y_stock_0, L_y_stock_1, L_y_stock_2, L_y_stock_3};

    int count_stock[] = {2, 2, 2, 2}; // Nombre de positions pour chaque article en stock

    // Liste des articles sélectionnés
    SelectedItem selected_items[MAX_ARTICLES_LISTE_ATTENTE];

    // Appel de la fonction
    int nb_selected = choose_items_stocks(item_names_requested, L_n_requested, count_requested,item_names_stock, L_n_stock, L_x_stock, L_y_stock, count_stock,selected_items);

    // Affichage des résultats
    printf("Articles sélectionnés :\n");
    for (int i = 0; i < nb_selected; i++) {
        printf("Article: %s\n", selected_items[i].item_name);
        printf("Positions: [ ");
        for (int j = 0; j < selected_items[i].count; j++) {
            printf("[%d, %d] ", selected_items[i].positions[j][0], selected_items[i].positions[j][1]);
        }
        printf("]\n");
        printf("Quantités: [ ");
        for (int j = 0; j < selected_items[i].count; j++) {
            printf("%d ", selected_items[i].quantities[j]);
        }
        printf("]\n");

        // Libération de la mémoire
        for (int j = 0; j < selected_items[i].count; j++) {
            free(selected_items[i].positions[j]);
        }
        free(selected_items[i].positions);
        free(selected_items[i].quantities);
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
    CU_add_test(suite, "Trajectoire de P25 à B5", test_trajectoire_P25_B5);
    CU_add_test(suite, "Trajectoire de B5 à P25", test_trajectoire_B5_P25);
    CU_add_test(suite, "Trajectoire de B10 à P25", test_trajectoire_B10_P25);
    CU_add_test(suite, "Trajectoire de D30 à B10", test_trajectoire_D30_B10);

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

    test_selection_items();

    return 0;
}
