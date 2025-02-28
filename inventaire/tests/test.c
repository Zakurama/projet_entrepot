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
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    init_stock(&stock, nb_rows, nb_columns);
    for (int i = 0; i < nb_rows; i++)
    {
        for (int j = 0; j < nb_columns; j++)
        {
            CU_ASSERT(stock[i][j]==STOCK_INIT)
        }
    }
}

void test_add_row(void){
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    int prev_nb_rows = nb_rows;
    init_stock(&stock, nb_rows, nb_columns);
    int nb_supplementary_rows = 3;
    add_row(&stock, &nb_rows, nb_columns, nb_supplementary_rows);
    CU_ASSERT(nb_rows == prev_nb_rows + nb_supplementary_rows);
}

void test_add_columns(void){
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    int prev_nb_columns = nb_columns;
    init_stock(&stock, nb_rows, nb_columns);
    int nb_supplementary_columns = 3;
    add_column(&stock, nb_rows, &nb_columns, nb_supplementary_columns);
    CU_ASSERT(nb_columns == prev_nb_columns + nb_supplementary_columns);
}

void test_add_stock(void){
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    init_stock(&stock, nb_rows, nb_columns);
    const int count = 2;
    int rows[2] = {1, 2};
    int columns[2] = {1, 2};
    int values[2] = {1, 2};
    char *error_message = modify_stock(&stock, nb_rows, nb_columns, rows, columns, values, count);
    CU_ASSERT(error_message == NULL);
    for (int i = 0; i < count; i++){
        CU_ASSERT(stock[rows[i]][columns[i]] == STOCK_INIT + values[i]);
    }
}

void test_add_stock_after_adding_size(void){
    int **stock;
    int nb_rows = 5;
    int nb_columns = 5;
    init_stock(&stock, nb_rows, nb_columns);

    const int count = 1;
    int nb_supplementary_columns = 3;
    int nb_supplementary_rows = 2;
    add_row(&stock, &nb_rows, nb_columns, nb_supplementary_rows);
    add_column(&stock, nb_rows, &nb_columns, nb_supplementary_columns);
    int rows[1] = {nb_rows - 1};
    int columns[1] = {nb_columns - 1};
    int values[1] = {1};
    
    char *error_message = modify_stock(&stock, nb_rows, nb_columns, rows, columns, values, count);
    CU_ASSERT(error_message == NULL);

    for (int i = 0; i < count; i++){
        CU_ASSERT(stock[rows[i]][columns[i]] == NEW_STOCK_INIT_VALUE + values[i]);
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

void test_handle_request_client(void){
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    init_stock(&stock, nb_rows, nb_columns);
    const char *request = "1_1.1,1_2.2";
    char *response = handle_request(&stock, nb_rows, nb_columns, request, 1);
    CU_ASSERT(response == NULL);
    CU_ASSERT(stock[0][0] == STOCK_INIT - 1);
    CU_ASSERT(stock[1][1] == STOCK_INIT - 1);
}

void test_handle_request_not_client(void){
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    init_stock(&stock, nb_rows, nb_columns);
    const char *request = "1_1.1,1_2.2";
    char *response = handle_request(&stock, nb_rows, nb_columns, request, 0);
    CU_ASSERT(response == NULL);
    CU_ASSERT(stock[0][0] == STOCK_INIT + 1);
    CU_ASSERT(stock[1][1] == STOCK_INIT + 1);
}

void test_handle_request_invalid_request(void) {
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    init_stock(&stock, nb_rows, nb_columns);
    const char *request = "invalid_request";
    char *response = handle_request(&stock, nb_rows, nb_columns, request, 1);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Invalid request format\n");
}

void test_handle_request_client_add_stock(void) {
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    init_stock(&stock, nb_rows, nb_columns);
    const char *request = "-1_1.1,-1_2.2";
    char *response = handle_request(&stock, nb_rows, nb_columns, request, 1);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Clients cannot add stock\n");
}

void test_handle_request_out_of_bounds(void) {
    int **stock;
    int nb_rows = 5;
    int nb_columns = 7;
    init_stock(&stock, nb_rows, nb_columns);
    const char *request = "6_1.1,1_8.2";
    char *response = handle_request(&stock, nb_rows, nb_columns, request, 0);
    CU_ASSERT(response != NULL);
    CU_ASSERT_STRING_EQUAL(response, "Invalid row or column index\n");
}

int main() {
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    pSuite = CU_add_suite("Test Suite", NULL, NULL);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test init stock", test_init_stock)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test add row", test_add_row)) {
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

    if (NULL == CU_add_test(pSuite, "test handle request client", test_handle_request_client)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle request not client", test_handle_request_not_client)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle request invalid request", test_handle_request_invalid_request)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle request client add stock", test_handle_request_client_add_stock)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "test handle request out of bounds", test_handle_request_out_of_bounds)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
