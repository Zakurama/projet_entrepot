#ifndef _INVENTAIRE_H
#define _INVENTAIRE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#include "utils.h"
#include "tcp.h"

#define LOCALPORT   3000
#define REMOTEIP "127.0.0.1"
#define REMOTEPORT 5000

#define STOCK_INIT  5
#define NEW_STOCK_INIT_VALUE 0

typedef struct{
    char *name;
    int **stock;
    int quantity;
} item_t;

typedef struct {
    int client_sd;
    int computer_sd;
    int * nb_rows;
    int * nb_columns;
    item_t ** items;
    int * nb_items;
} thread_args_t;

void init_stock(item_t *item, int nb_rows, int nb_columns, const char *item_placement);
char *get_stock_string(int **stock, int nb_rows, int nb_columns) ;
void print_stock(int **stock, int nb_rows, int nb_columns);
void add_row(item_t *items, int nb_items, int *nb_rows, int nb_columns, int nb_supplementary_rows);
void add_column(item_t *items, int nb_items, int nb_rows, int *nb_columns, int nb_supplementary_columns);
char * parse_message(const char *request, int *L_n, int *L_x, int *L_y, int *count, int max_elements);
char * modify_stock(item_t *item, int nb_rows, int nb_columns, int *rows, int *columns, int *values, int count);
void free_stock(int **stock, int nb_rows);
void *handle_client(void *arg);
void *stock_manager(void *arg);
char *check_client_request(const char *request, item_t *items, int nb_items, int max_elements);
int get_item_index(item_t *items, int nb_items, const char *name);
void add_item(item_t **items, int *nb_items, item_t item, int *nb_columns, int *nb_rows); 
char *transfer_stock(item_t *items, int nb_items, int nb_rows, int nb_columns, const char **item_names, int nb_items_request);
char *handle_items_request(item_t *items, int nb_items, int nb_rows, int nb_columns, const char *request, int central_coputer);
char **parse_items_names(item_t *items, int nb_items, const char *request, int *nb_items_request);
char *parse_client_request(const char *request, int max_elements, int L_n[max_elements], char *item_names[max_elements], int *count);

#endif
