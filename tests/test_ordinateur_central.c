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

    int nb_colonnes = 4;
    int nb_lignes = 4;
    trajectoire(pos_initiale, pos_finale, path, nb_lignes, nb_colonnes);

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

void test_selection_items(void) {
    // Liste des articles demandés par le client
    char *item_names_requested[] = {"Banane", "Pomme", "Raisin"};
    int L_n_requested[] = {6, 5, 8}; // Quantités demandées
    int count_requested = 3;

    // Stock disponible
    char *item_names_stock[] = {"Banane", "Pomme", "Orange", "Raisin"};
    int L_n_stock_0[] = {5, 6};  // Stock de "Banane" (2 positions)
    int L_n_stock_1[] = {3, 2};  // Stock de "Pomme" (2 positions)
    int L_n_stock_2[] = {4, 7};  // Stock de "Orange" (non demandé)
    int L_n_stock_3[] = {2, 6};  // Stock de "Raisin" (2 positions)

    int *L_n_stock[] = {L_n_stock_0, L_n_stock_1, L_n_stock_2, L_n_stock_3};

    // Positions des articles en stock (allées et bacs)
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

    int count_stock[] = {2, 2, 2, 2}; // Nombre de positions par article en stock

    // Liste des articles sélectionnés
    Item_selected selected_items[MAX_ARTICLES_LISTE_ATTENTE] = {0}; // Initialisation à zéro

    // Appel de la fonction
    int nb_selected = choose_items_stocks(
        item_names_requested, L_n_requested, count_requested,
        item_names_stock, L_n_stock, L_x_stock, L_y_stock, count_stock,
        selected_items
    );

    // Vérifications avec CUnit
    CU_ASSERT_EQUAL(nb_selected, 3); // Vérifier que 3 articles ont été sélectionnés

    // Vérification du premier article (Banane)
    CU_ASSERT_STRING_EQUAL(selected_items[0].item_name, "Banane");
    CU_ASSERT_EQUAL(selected_items[0].count, 2);
    CU_ASSERT_EQUAL(selected_items[0].positions[0][0], 1);
    CU_ASSERT_EQUAL(selected_items[0].positions[0][1], 10);
    CU_ASSERT_EQUAL(selected_items[0].positions[1][0], 2);
    CU_ASSERT_EQUAL(selected_items[0].positions[1][1], 11);
    CU_ASSERT_EQUAL(selected_items[0].quantities[0], 5);
    CU_ASSERT_EQUAL(selected_items[0].quantities[1], 1); // La somme doit faire 6

    // Vérification du second article (Pomme)
    CU_ASSERT_STRING_EQUAL(selected_items[1].item_name, "Pomme");
    CU_ASSERT_EQUAL(selected_items[1].count, 2);
    CU_ASSERT_EQUAL(selected_items[1].positions[0][0], 3);
    CU_ASSERT_EQUAL(selected_items[1].positions[0][1], 12);
    CU_ASSERT_EQUAL(selected_items[1].positions[1][0], 4);
    CU_ASSERT_EQUAL(selected_items[1].positions[1][1], 13);
    CU_ASSERT_EQUAL(selected_items[1].quantities[0], 3);
    CU_ASSERT_EQUAL(selected_items[1].quantities[1], 2); // La somme doit faire 5

    // Vérification du troisième article (Raisin)
    CU_ASSERT_STRING_EQUAL(selected_items[2].item_name, "Raisin");
    CU_ASSERT_EQUAL(selected_items[2].count, 2);
    CU_ASSERT_EQUAL(selected_items[2].positions[0][0], 7);
    CU_ASSERT_EQUAL(selected_items[2].positions[0][1], 16);
    CU_ASSERT_EQUAL(selected_items[2].positions[1][0], 8);
    CU_ASSERT_EQUAL(selected_items[2].positions[1][1], 17);
    CU_ASSERT_EQUAL(selected_items[2].quantities[0], 2);
    CU_ASSERT_EQUAL(selected_items[2].quantities[1], 6); // La somme doit faire 8

    // Libération de la mémoire allouée dans `selected_items`
    for (int i = 0; i < nb_selected; i++) {
        for (int j = 0; j < selected_items[i].count; j++) {
            free(selected_items[i].positions[j]);
        }
        free(selected_items[i].positions);
        free(selected_items[i].quantities);
    }
}

void test_update_shared_memory_stock(void) {
    // Initialisation de la structure Robot
    Robot robot = {0}; // Initialise tous les pointeurs à NULL

    // Initialisation et allocation de Item
    Item_selected item;
    item.item_name = "Item1";
    item.count = 1; // Un seul point sélectionné

    // Allocation de mémoire pour positions et quantities
    item.positions = (int **)malloc(item.count * sizeof(int *));
    item.quantities = (int *)malloc(item.count * sizeof(int));

    // Vérification de l'allocation
    CU_ASSERT_PTR_NOT_NULL_FATAL(item.positions);
    CU_ASSERT_PTR_NOT_NULL_FATAL(item.quantities);

    // Allocation pour chaque position [x, y]
    item.positions[0] = (int *)malloc(2 * sizeof(int));
    CU_ASSERT_PTR_NOT_NULL_FATAL(item.positions[0]);

    // Initialisation des valeurs
    item.positions[0][0] = 5;
    item.positions[0][1] = 10;
    item.quantities[0] = 3;

    // Appel de la fonction
    update_shared_memory_stock(&robot, item,0);

    // Vérification que l'élément a été ajouté correctement
    CU_ASSERT_PTR_NOT_NULL(robot.item_name[0]);
    CU_ASSERT_STRING_EQUAL(robot.item_name[0], "Item1");

    CU_ASSERT_PTR_NOT_NULL(robot.positions[0]);
    CU_ASSERT_EQUAL(robot.positions[0][0], 6);
    CU_ASSERT_EQUAL(robot.positions[0][1], 11);

    CU_ASSERT_PTR_NOT_NULL(robot.quantities[0]);
    CU_ASSERT_EQUAL(robot.quantities[0][0], 3);

    // Libération de la mémoire allouée
    free(item.positions[0]);
    free(item.positions);
    free(item.quantities);
    free(robot.positions[0]);
    free(robot.quantities[0]);
}

void test_create_inventory_string(void) {
    int max_elements = 50;
    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    char *item_names[max_elements];
    int count[max_elements];
    int nb_items = 3;

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    // Initialize test data
    count[0] = 2;
    strcpy(item_names[0], "item0");
    L_n[0][0] = 2; L_x[0][0] = 1; L_y[0][0] = 1;
    L_n[0][1] = 2; L_x[0][1] = 2; L_y[0][1] = 2;

    count[1] = 1;
    strcpy(item_names[1], "item1");
    L_n[1][0] = 1; L_x[1][0] = 3; L_y[1][0] = 3;

    count[2] = 1;
    strcpy(item_names[2], "item2");
    L_n[2][0] = 1; L_x[2][0] = 4; L_y[2][0] = 4;

    char *inventory_string = create_inventory_string(nb_items, max_elements, count, L_n, L_x, L_y, item_names);

    // Validate results
    CU_ASSERT_STRING_EQUAL(inventory_string, "item0;2_1.1,2_2.2/item1;1_3.3/item2;1_4.4");

    // Cleanup memory
    free(inventory_string);
    for (int i = 0; i < max_elements; i++) {
        free(L_n[i]);
        free(L_x[i]);
        free(L_y[i]);
        free(item_names[i]);
    }
}

void test_create_inventory_string_empty(void) {
    int max_elements = 50;
    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    char *item_names[max_elements];
    int count[max_elements];
    int nb_items = 0;

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    char *inventory_string = create_inventory_string(nb_items, max_elements, count, L_n, L_x, L_y, item_names);

    // Validate results
    CU_ASSERT_STRING_EQUAL(inventory_string, "");

    // Cleanup memory
    free(inventory_string);
    for (int i = 0; i < max_elements; i++) {
        free(L_n[i]);
        free(L_x[i]);
        free(L_y[i]);
        free(item_names[i]);
    }
}

void test_create_inventory_string_single_item(void) {
    int max_elements = 50;
    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    char *item_names[max_elements];
    int count[max_elements];
    int nb_items = 1;

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        item_names[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    // Initialize test data
    count[0] = 1;
    strcpy(item_names[0], "item0");
    L_n[0][0] = 2; L_x[0][0] = 1; L_y[0][0] = 1;

    char *inventory_string = create_inventory_string(nb_items, max_elements, count, L_n, L_x, L_y, item_names);

    // Validate results
    CU_ASSERT_STRING_EQUAL(inventory_string, "item0;2_1.1");

    // Cleanup memory
    free(inventory_string);
    for (int i = 0; i < max_elements; i++) {
        free(L_n[i]);
        free(L_x[i]);
        free(L_y[i]);
        free(item_names[i]);
    }

}

void test_authorize_robot_connexion_authorized(void) {
    char *file_name = "temp_robots.csv";
    char *robot_ip = "192.168.1.1";

    // Create a temporary CSV file for testing
    FILE *file = fopen(file_name, "w");
    fprintf(file, "robot_ip,robot_id\n192.168.1.1,1001\n192.168.1.2,1002\n");
    fclose(file);

    int robot_id = authorize_robot_connexion(file_name, robot_ip);

    CU_ASSERT_EQUAL(robot_id, 1001);

    // Remove the temporary file
    remove(file_name);
}

void test_authorize_robot_connexion_not_authorized(void) {
    char *file_name = "temp_robots.csv";
    char *robot_ip = "192.168.1.3";

    // Create a temporary CSV file for testing
    FILE *file = fopen(file_name, "w");
    fprintf(file, "robot_ip,robot_id\n192.168.1.1,1001\n192.168.1.2,1002\n");
    fclose(file);

    int robot_id = authorize_robot_connexion(file_name, robot_ip);

    CU_ASSERT_EQUAL(robot_id, 0);

    // Remove the temporary file
    remove(file_name);
}

void test_authorize_robot_connexion_invalid_file(void) {
    char *file_name = "invalid.csv";
    char *robot_ip = "192.168.1.1";

    int robot_id = authorize_robot_connexion(file_name, robot_ip);

    CU_ASSERT_EQUAL(robot_id, -1);
}

void test_authorize_robot_connexion_empty_file(void) {
    char *file_name = "empty.csv";
    char *robot_ip = "192.168.1.1";

    // Create an empty CSV file for testing
    FILE *file = fopen(file_name, "w");
    fclose(file);

    int robot_id = authorize_robot_connexion(file_name, robot_ip);

    CU_ASSERT_EQUAL(robot_id, 0);

    // Remove the temporary file
    remove(file_name);
}
void test_name_waypoints_creation(void){
    Liste_pos_waypoints liste_waypoints;
    name_waypoints_creation(&liste_waypoints, 2, 3, 2);

    int total_length = 0;
    
    for (int i = 0; liste_waypoints.name_waypoints[i][0] != '\0'; i++) 
    {
        total_length += strlen(liste_waypoints.name_waypoints[i]); 
    }

    CU_ASSERT(total_length != 0);    

    char buffer[MAX_WAYPOINTS];

    int i = 0; 
    for (i=0; liste_waypoints.name_waypoints[i][0] != '\0'; i++) {
        strcat(buffer, liste_waypoints.name_waypoints[i]);
    }

    CU_ASSERT(i == 22); //nb_waypoints
    CU_ASSERT_STRING_EQUAL(buffer, "M3M6M9M12M15M18D3D6D9D12D15D18S7S8S13S14S19S20B3B9P15P18");   
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

    if (NULL == CU_add_test(suite, "test add item in shared memory", test_update_shared_memory_stock)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test select item in stocks", test_selection_items)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test create inventory string", test_create_inventory_string)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test create inventory string empty", test_create_inventory_string_empty)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test create inventory string single item", test_create_inventory_string_single_item)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test add item in shared memory", test_update_shared_memory_stock)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test select item in stocks", test_selection_items)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test create inventory string", test_create_inventory_string)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test create inventory string empty", test_create_inventory_string_empty)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test create inventory string single item", test_create_inventory_string_single_item)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test authorize robot connexion authorized", test_authorize_robot_connexion_authorized)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test authorize robot connexion not authorized", test_authorize_robot_connexion_not_authorized)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test authorize robot connexion invalid file", test_authorize_robot_connexion_invalid_file)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(suite, "test authorize robot connexion empty file", test_authorize_robot_connexion_empty_file)) {
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    if (NULL == CU_add_test(suite, "test name waypoints invalid", test_name_waypoints_creation)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}
