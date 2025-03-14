#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "tcp.h"
#include "utils.h"
#include "inventaire.h"

void test_init_stock(void){
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    item_t item;
    item.stock = NULL;
    init_stock(&item, nb_rows, nb_columns, item_placement);
    CU_ASSERT(item.quantity == 10);

    for (int i = 0; i < nb_rows; i++)
    {
        for (int j = 0; j < nb_columns; j++)
        {
            if (i == 0 && j == 0){
                CU_ASSERT(item.stock[i][j]== 5)
            }
            else if (i == 1 && j == 1){
                CU_ASSERT(item.stock[i][j]== 5)
            }
            else{
                CU_ASSERT(item.stock[i][j]==0)
            }
        }
    }
}

void test_add_rows(void) {
    int nb_rows = 5;
    int nb_columns = 7;
    int prev_nb_rows = nb_rows;
    const char *item_placement = "5_1.1,5_2.2";

    // Allocate and initialize items
    item_t items[2];
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL; // Ensure stock is initialized to NULL before allocation
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
        items[i].quantity = 5;
    }

    int nb_supplementary_rows = 3;
    add_row(items, 2, &nb_rows, nb_columns, nb_supplementary_rows);
    CU_ASSERT(nb_rows == prev_nb_rows + nb_supplementary_rows);

    for (int i = prev_nb_rows; i < nb_rows; i++) {
        for (int j = 0; j < nb_columns; j++) {
            CU_ASSERT(items[0].stock[i][j] == NEW_STOCK_INIT_VALUE);
            CU_ASSERT(items[1].stock[i][j] == NEW_STOCK_INIT_VALUE);
        }
    }

    for (int i = 0; i < 2; i++) {
        free_stock(items[i].stock, nb_rows);
    }
}

void test_add_columns(void) {
    int nb_rows = 5;
    int nb_columns = 7;
    int prev_nb_columns = nb_columns;
    const char *item_placement = "5_1.1,5_2.2";

    // Allocate and initialize items
    item_t items[2];
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL; // Ensure stock is initialized
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
        items[i].quantity = 5;
    }

    int nb_supplementary_columns = 3;
    add_column(items, 2, nb_rows, &nb_columns, nb_supplementary_columns);
    CU_ASSERT(nb_columns == prev_nb_columns + nb_supplementary_columns);

    // Verify that new columns are initialized correctly
    for (int k = 0; k < 2; k++) {
        for (int i = 0; i < nb_rows; i++) {
            for (int j = prev_nb_columns; j < nb_columns; j++) {
                CU_ASSERT(items[k].stock[i][j] == NEW_STOCK_INIT_VALUE);
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        free_stock(items[i].stock, nb_rows);
    }
    
}

void test_add_stock(void){
    int nb_rows = 5;
    int nb_columns = 7;
    int stock_init = 5;
    int rows[2] = {0, 1};
    int columns[2] = {0, 1};
    char item_placement[100];
    sprintf(item_placement, "%d_%d.%d,%d_%d.%d", stock_init, rows[0]+1, columns[0]+1, stock_init, rows[1]+1, columns[1]+1);

    item_t item;
    item.stock = NULL;
    init_stock(&item, nb_rows, nb_columns, item_placement);

    const int count = 2;
    int values[2] = {1, 2};
    char *error_message = modify_stock(&item, nb_rows, nb_columns, rows, columns, values, count);
    CU_ASSERT(error_message == NULL);
    for (int i = 0; i < count; i++){
        CU_ASSERT(item.stock[rows[i]][columns[i]] == 5 + values[i]);
    }
}

void test_add_stock_after_adding_size(void){
    int nb_rows = 5;
    int nb_columns = 5;
    const char *item_placement = "5_1.1,5_2.2";
    item_t items[1];
    items[0].stock = NULL;
    init_stock(&items[0], nb_rows, nb_columns, item_placement);

    const int count = 1;
    int nb_supplementary_columns = 3;
    int nb_supplementary_rows = 2;
    add_row(items, count, &nb_rows, nb_columns, nb_supplementary_rows);
    add_column(items, count, nb_rows, &nb_columns, nb_supplementary_columns);
    int rows[1] = {nb_rows - 1};
    int columns[1] = {nb_columns - 1};
    int values[1] = {1};
    
    char *error_message = modify_stock(&items[0], nb_rows, nb_columns, rows, columns, values, count);
    CU_ASSERT(error_message == NULL);
    for (int i = 0; i < count; i++){
        CU_ASSERT(items[0].stock[rows[i]][columns[i]] == NEW_STOCK_INIT_VALUE + values[i]);
    }
}

void test_parse_message(void){
    const char *request = "1_1.1,1_2.2";
    int *L_n = malloc(50 * sizeof(int));
    int *L_x = malloc(50 * sizeof(int));
    int *L_y = malloc(50 * sizeof(int)); 
    int count;
    char *response = parse_message(request, L_n, L_x, L_y, &count, 2);
    CU_ASSERT(response == NULL);
    CU_ASSERT(L_n[0] == 1);
    CU_ASSERT(L_x[0] == 1);
    CU_ASSERT(L_y[0] == 1);
    CU_ASSERT(L_n[1] == 1);
    CU_ASSERT(L_x[1] == 2);
    CU_ASSERT(L_y[1] == 2);
}

void test_check_client_message_correct(void) {
    const char *request = "item1_1,item2_2";
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    char *response = check_client_request(request, items, 2, 2);
    CU_ASSERT(response == NULL);
}

void test_check_client_message_item_not_found(void) {
    const char *request = "item1_1,item2_2";
    item_t items[1];
    items[0].name = "item1";
    items[0].quantity = 5;
    char *response = check_client_request(request, items, 1, 2);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Item not found\n");
}

void test_check_client_message_invalid_request(void) {
    const char *request = "item1.1,item2_2"; // . instead of _
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    char *response = check_client_request(request, items, 2, 2);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Invalid request format\n");
}

void test_check_client_message_negative_quantity(void) {
    const char *request = "item1_-1,item2_2";
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    char *response = check_client_request(request, items, 2, 2);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Clients cannot add stock\n");
}

void test_check_client_message_insufficient_quantity(void) {
    const char *request = "item1_6,item2_2";
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    char *response = check_client_request(request, items, 2, 2);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Cannot take 6 of stock item1 at current value 5\n");
}

void test_check_client_message_max_elements_exceeded(void) {
    const char *request = "item1_1,item2_2";
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    char *response = check_client_request(request, items, 2, 1); // max_elements = 1
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Maximum number of elements exceeded, please reduce the number to change\n");
}

void test_check_client_message_duplicate_item_name(void) {
    const char *request = "item1_1,item1_2";
    item_t items[1];
    items[0].name = "item1";
    items[0].quantity = 5;
    char *response = check_client_request(request, items, 1, 2);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Duplicate item name in request\n");
}

void test_add_item(void) {
    item_t *items = NULL;
    int nb_items = 0;
    int nb_columns = 5;
    int nb_rows = 5;
    item_t item1 = {"item1", NULL, 0};
    item_t item2 = {"item2", NULL, 0};
    add_item(&items, &nb_items, item1, &nb_columns, &nb_rows);
    add_item(&items, &nb_items, item2, &nb_columns, &nb_rows);
    CU_ASSERT(nb_items == 2);
    CU_ASSERT_STRING_EQUAL(items[0].name, "item1");
    CU_ASSERT(items[0].quantity == 0);
    CU_ASSERT(items[0].stock != NULL);
    CU_ASSERT_STRING_EQUAL(items[1].name, "item2");
    CU_ASSERT(items[1].quantity == 0);
    CU_ASSERT(items[1].stock != NULL);
}

void test_get_item_index(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    CU_ASSERT(get_item_index(items, 2, "item1") == 0);
    CU_ASSERT(get_item_index(items, 2, "item2") == 1);
    CU_ASSERT(get_item_index(items, 2, "item3") == -1);
}

void test_transfer_stock(void){
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL; // Ensure stock is initialized to NULL before allocation
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
    }
    const char *item_names[2] = {"item1", "item2"};
    char *response = transfer_stock(items, 2, nb_rows, nb_columns, item_names, 2);
    free_stock(items[0].stock, nb_rows);
    free_stock(items[1].stock, nb_rows);
    CU_ASSERT_STRING_EQUAL(response, "item1;5_1.1,5_2.2/item2;5_1.1,5_2.2");
}

void test_handle_items_request_correct(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL;
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
    }
    const char *request = "item1;1_1.1,1_2.2/item2;2_1.1,2_2.2";
    char *response = handle_items_request(items, 2, nb_rows, nb_columns, request, 0);
    CU_ASSERT(response == NULL);
    CU_ASSERT(items[0].stock[0][0] == 6);
    CU_ASSERT(items[0].stock[1][1] == 6);
    CU_ASSERT(items[1].stock[0][0] == 7);
    CU_ASSERT(items[1].stock[1][1] == 7);

    for (int i = 0; i < 2; i++) {
        free_stock(items[i].stock, nb_rows);
    }
}

void test_handle_items_request_correct_not_central_computer(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL;
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
    }
    const char *request = "item1;1_1.1,1_2.2/item2;2_1.1,2_2.2";
    char *response = handle_items_request(items, 2, nb_rows, nb_columns, request, 1);
    CU_ASSERT(response == NULL);
    CU_ASSERT(items[0].stock[0][0] == 4);
    CU_ASSERT(items[0].stock[1][1] == 4);
    CU_ASSERT(items[1].stock[0][0] == 3);
    CU_ASSERT(items[1].stock[1][1] == 3);

    for (int i = 0; i < 2; i++) {
        free_stock(items[i].stock, nb_rows);
    }
}

void test_handle_items_request_invalid_format(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL;
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
    }
    const char *request = "item1;1_1.1,1_2.2/item2";
    char *response = handle_items_request(items, 2, nb_rows, nb_columns, request, 0);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Invalid request format\n");
    for (int i = 0; i < 2; i++) {
        free_stock(items[i].stock, nb_rows);
    }
}

void test_handle_items_request_item_not_found(void) {
    item_t items[1];
    items[0].name = "item1";
    items[0].quantity = 5;
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    items[0].stock = NULL;
    init_stock(&items[0], nb_rows, nb_columns, item_placement);
    const char *request = "item2;1_1.1,1_2.2";
    char *response = handle_items_request(items, 1, nb_rows, nb_columns, request, 1);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Item not found\n");
    free_stock(items[0].stock, nb_rows);
}

void test_handle_items_request_parse_error(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL;
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
    }
    const char *request = "item1;1_1.1,1_2.2/item2;invalid";
    char *response = handle_items_request(items, 2, nb_rows, nb_columns, request, 0);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Invalid request format\n");
    for (int i = 0; i < 2; i++) {
        free_stock(items[i].stock, nb_rows);
    }
}

void test_handle_items_request_modify_stock_error(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    items[0].quantity = 5;
    items[1].quantity = 5;
    int nb_rows = 5;
    int nb_columns = 7;
    const char *item_placement = "5_1.1,5_2.2";
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL;
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
    }
    const char *request = "item1;1_1.1,1_2.2/item2;1_6.1";
    char *response = handle_items_request(items, 2, nb_rows, nb_columns, request, 0);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Invalid row or column index\n");
    CU_ASSERT(items[0].stock[0][0] == 5);
    CU_ASSERT(items[0].stock[1][1] == 5);
    CU_ASSERT(items[1].stock[0][0] == 5);
    CU_ASSERT(items[1].stock[1][1] == 5);

    for (int i = 0; i < 2; i++) {
        free_stock(items[i].stock, nb_rows);
    }
}

void test_parse_items_names_correct(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    const char *request = "item1;1_1.1,1_2.2/item2;2_1.1,2_2.2";
    int nb_items_request;
    char **item_names = parse_items_names(items, 2, request, &nb_items_request);
    CU_ASSERT(item_names != NULL);
    CU_ASSERT(nb_items_request == 2);
    CU_ASSERT_STRING_EQUAL(item_names[0], "item1");
    CU_ASSERT_STRING_EQUAL(item_names[1], "item2");
    for (int i = 0; i < nb_items_request; i++) {
        free(item_names[i]);
    }
    free(item_names);
}

void test_parse_items_names_invalid_format(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    const char *request = "item1;1_1.1,1_2.2/item2;2_1.1,2_2.2";
    int nb_items_request;
    char **item_names = parse_items_names(items, 2, request, &nb_items_request);
    CU_ASSERT_NOT_EQUAL(item_names, NULL);
    CU_ASSERT_EQUAL(nb_items_request, 2);
    if (item_names != NULL){
        CU_ASSERT_STRING_EQUAL(item_names[0], "item1");
        CU_ASSERT_STRING_EQUAL(item_names[1], "item2")
    }
}

void test_parse_items_names_empty_request(void) {
    item_t items[2];
    items[0].name = "item1";
    items[1].name = "item2";
    const char *request = "";
    int nb_items_request;
    char **item_names = parse_items_names(items, 2, request, &nb_items_request);
    CU_ASSERT(item_names == NULL);
}

void test_parse_items_names_exceed_max_items(void) {
    item_t items[51];
    for (int i = 0; i < 51; i++) {
        items[i].name = malloc(10);
        sprintf(items[i].name, "item%d", i + 1);
    }
    char request[1024] = "";
    for (int i = 0; i < 51; i++) {
        char temp[20];
        sprintf(temp, "item%d;1_1.1", i + 1);
        strcat(request, temp);
        if (i < 50) {
            strcat(request, "/");
        }
    }
    int nb_items_request;
    char **item_names = parse_items_names(items, 51, request, &nb_items_request);
    CU_ASSERT(item_names == NULL);
    for (int i = 0; i < 51; i++) {
        free(items[i].name);
    }
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

int main() {
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("Test Inventaire", NULL, NULL);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test init stock", test_init_stock)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test add rows", test_add_rows)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test add columns", test_add_columns)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test add stock", test_add_stock)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test add stock after adding size", test_add_stock_after_adding_size)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse message", test_parse_message)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test check client message correct", test_check_client_message_correct)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test check client message item not found", test_check_client_message_item_not_found)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test check client message invalid request", test_check_client_message_invalid_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test check client message negative quantity", test_check_client_message_negative_quantity)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test check client message insufficient quantity", test_check_client_message_insufficient_quantity)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test check client message max elements exceeded", test_check_client_message_max_elements_exceeded)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test check client message duplicate item name", test_check_client_message_duplicate_item_name)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test add item", test_add_item)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test get item index", test_get_item_index)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test transfer stock", test_transfer_stock)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle items request correct", test_handle_items_request_correct)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle items request correct not central computer", test_handle_items_request_correct_not_central_computer)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle items request invalid format", test_handle_items_request_invalid_format)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle items request item not found", test_handle_items_request_item_not_found)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle items request parse error", test_handle_items_request_parse_error)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle items request modify stock error", test_handle_items_request_modify_stock_error)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse items names correct", test_parse_items_names_correct)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse items names invalid format", test_parse_items_names_invalid_format)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse items names empty request", test_parse_items_names_empty_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse items names exceed max items", test_parse_items_names_exceed_max_items)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse client good request", test_parse_client_request_good_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse client invalid format", test_parse_client_request_invalid_format)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse client too many items", test_parse_client_request_too_many_items)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test parse client empty request", test_parse_client_request_empty_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
