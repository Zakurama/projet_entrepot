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

#define STOCK_INIT  5
#define NEW_STOCK_INIT_VALUE 0

typedef struct {
    int client_sd;
    int * nb_rows;
    int * nb_columns;
    int *** stock;
} thread_args_t;

void init_stock(int ***stock, int nb_rows, int nb_columns);
char *get_stock_string(int **stock, int nb_rows, int nb_columns) ;
void print_stock(int **stock, int nb_rows, int nb_columns);
void add_row(int ***stock, int *nb_rows, int nb_columns, int nb_supplementary_rows);
void add_column(int ***stock, int nb_rows, int *nb_columns, int nb_supplementary_columns);
char * handle_request(int ***stock, int nb_rows, int nb_columns, const char *request, int client);
char * parse_message(const char *request, int *L_n, int *L_x, int *L_y, int *count, int max_elements);
char * modify_stock(int ***stock, int nb_rows, int nb_columns, int *rows, int *columns, int *values, int count);
void free_stock(int **stock, int nb_rows);
void *handle_client(void *arg);
void *stock_manager(void *arg);

#endif
