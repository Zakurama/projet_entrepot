#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#include "utils.h"
#include "tcp.h"

#define LOCALPORT   3000

#define STOCK_INIT  5

pthread_mutex_t stock_mutex = PTHREAD_MUTEX_INITIALIZER;
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

void init_stock(int ***stock, int nb_rows, int nb_columns) {
    *stock = (int **)malloc(nb_rows * sizeof(int *));
    CHECK_ERROR(*stock, NULL, "Failed to allocate memory for rows");

    for (int i = 0; i < nb_rows; i++) {
        (*stock)[i] = (int *)malloc(nb_columns * sizeof(int));
        CHECK_ERROR((*stock)[i], NULL, "Failed to allocate memory for columns");

        // Initialize elements to 0
        for (int j = 0; j < nb_columns; j++) {
            (*stock)[i][j] = STOCK_INIT;
        }
    }
}

char *get_stock_string(int **stock, int nb_rows, int nb_columns) {
    // Estimation de la taille nécessaire
    int buffer_size = (nb_rows + 1) * (nb_columns + 1) * 10 + 100;
    char *buffer = malloc(buffer_size);
    CHECK_ERROR(buffer, NULL, "Failed to allocate memory for buffer");

    char *ptr = buffer;
    ptr += sprintf(ptr, "\n");

    // Ligne des numéros de colonne
    for (int j = 0; j < nb_columns; j++) {
        ptr += sprintf(ptr, "\t%d", j + 1);
    }
    ptr += sprintf(ptr, "\n\n");

    // Contenu de la grille
    for (int i = 0; i < nb_rows; i++) {
        ptr += sprintf(ptr, "%d\t", i + 1);
        for (int j = 0; j < nb_columns; j++) {
            ptr += sprintf(ptr, "%d\t", stock[i][j]);
        }
        ptr += sprintf(ptr, "\n");
    }

    return buffer;
}

void print_stock(int **stock, int nb_rows, int nb_columns){
    printf("%s", get_stock_string(stock, nb_rows, nb_columns));
}

void add_row(int ***stock, int *nb_rows, int nb_columns, int nb_supplementary_rows) {
    *stock = (int **)realloc(*stock, (*nb_rows + nb_supplementary_rows) * sizeof(int *));
    CHECK_ERROR(*stock, NULL, "Failed to allocate memory for new row");

    // Allocate memory for the new rows
    for (int i = 0; i < nb_supplementary_rows; i++){
        (*stock)[*nb_rows + i] = (int *)malloc(nb_columns * sizeof(int));
        CHECK_ERROR((*stock)[*nb_rows + i], NULL, "Failed to allocate memory for new row");
    }
    

    // Initialize new row to 0
    for (int i = 0; i < nb_supplementary_rows; i++) {
        for (int j = 0; j < nb_columns; j++) {
            (*stock)[*nb_rows+i][j] = 0;
        }
    }

    (*nb_rows) = *nb_rows + nb_supplementary_rows; // Increment row count
}

void add_column(int ***stock, int nb_rows, int *nb_columns, int nb_supplementary_columns) {
    for (int i = 0; i < nb_rows; i++) {
        (*stock)[i] = (int *)realloc((*stock)[i], (*nb_columns + nb_supplementary_columns) * sizeof(int));
        CHECK_ERROR((*stock)[i], NULL, "Failed to allocate memory for new column");
        for (int j = *nb_columns; j < *nb_columns + nb_supplementary_columns; j++)
        (*stock)[i][j] = 0; // Initialize new column to 0
    }

    (*nb_columns) = *nb_columns + nb_supplementary_columns; // Increment column count
}

// client is a boolean that indicates if the request is coming from a client or a stock manager
char * handle_request(int ***stock, int nb_rows, int nb_columns, const char *request, int client){
    int max_elements = 50;
    int L_n[max_elements], L_x[max_elements], L_y[max_elements], count;
    char * return_parse_message = parse_message(request, L_n, L_x, L_y, &count, max_elements);
    if (return_parse_message != NULL) {
        return return_parse_message;
    }

    for (int i = 0; i < count; i++) {
        // humans start counting from 1
        L_x[i]--;
        L_y[i]--;
        if (client){
            // clients do not add but remove stock
            L_n[i] = - L_n[i];
            if (L_n[i] > 0) {
                return "Clients cannot add stock\n";
            }
        }
    }

    char * return_modify_stock_message = modify_stock(stock, nb_columns, nb_rows, L_x, L_y, L_n, count);
    if (return_modify_stock_message != NULL) {
        return return_modify_stock_message;
    }
    return NULL;
}

// message format: "N_X.Y,..." 
char * parse_message(const char *request, int *L_n, int *L_x, int *L_y, int *count, int max_elements) {
    *count = 0;  // Initialize count

    char temp[strlen(request) + 1];
    strcpy(temp, request); // Create a modifiable copy of the string

    char *token = strtok(temp, ",");
    while (token != NULL) {
        if (*count >= max_elements) {
            return "Maximum number of elements exceeded, please reduce the number number to change\n";
        }
        sscanf(token, "%d_%d.%d", &L_n[*count], &L_x[*count], &L_y[*count]);
        (*count)++;
        token = strtok(NULL, ",");
    }
    return NULL;
}

char * modify_stock(int ***stock, int nb_rows, int nb_columns, int *rows, int *columns, int *values, int count) {
    for (int i = 0; i < count; i++) {
        if (rows[i] < 0 || rows[i] >= nb_rows || columns[i] < 0 || columns[i] >= nb_columns) {
            return "Invalid row or column index\n";
        }
        else if ((*stock)[rows[i]][columns[i]] + values[i] < 0) {
            char *error_message = (char *)malloc(100 * sizeof(char));
            snprintf(error_message, 100, "Cannot add %d of stock (%d,%d) at current value %d\n", values[i], rows[i], columns[i], (*stock)[rows[i]][columns[i]]);
            return error_message;
        }
    }
    for (int i = 0; i < count; i++) {
        (*stock)[rows[i]][columns[i]] += values[i];
    }
    return NULL;
}

void free_stock(int **stock, int nb_rows) {
    for (int i = 0; i < nb_rows; i++) {
        free(stock[i]);
    }
    free(stock);
}

void *handle_client(void *arg) {
    
    thread_args_t *args = (thread_args_t *)arg;
    int client_sd = args->client_sd;
    int *nb_rows = args->nb_rows;  // Récupère le pointeur vers nb_rows
    int *nb_columns = args->nb_columns;  // Récupère le pointeur vers nb_columns
    int ***stock = args->stock;  // Récupère le pointeur vers stock
    free(args);  // Libère la mémoire de la structure allouée

    char buff_reception[MAXOCTETS + 1];
    char buff_emission[MAXOCTETS + 1];

    while (1) {
        // Réception du message de la part du lecteur
        recev_message(client_sd,buff_reception);

        if (strcmp(buff_reception, "stock") == 0) {
            strcpy(buff_emission, get_stock_string(*stock, *nb_rows, *nb_columns));
        }
        else {
            pthread_mutex_lock(&stock_mutex);
            char * message = handle_request(stock, *nb_rows, *nb_columns, buff_reception, 1);
            pthread_mutex_unlock(&stock_mutex);

            if (message != NULL) {
                strcpy(buff_emission, message);
            }
            else {
                strcpy(buff_emission, "Stock updated successfully\n");
            }
            
        }
        send_message(client_sd,buff_emission);
    }
    close_socket(&client_sd);
    return NULL;
}

int main(int argc, char *argv[]) {
    int nb_columns = 5;
    int nb_rows = 5;
    int **stock;

    init_stock(&stock, nb_rows, nb_columns);

    int se;
    int client_sd;

    if (argc == 2){
        init_tcp_socket(&se, argv[1], LOCALPORT,1);
    } 
    else if (argc == 3){ 
        init_tcp_socket(&se, argv[1], (u_int16_t) atoi(argv[2]),1);
    }
    else {
        fprintf(stderr, "Usage: %s <local_ip> [<local_port>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Communication avec les clients

    listen_to(se);

    while (1) {
        client_sd = accept_client(se);
        
        pthread_t client_thread;
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->nb_rows = &nb_rows;
        args->nb_columns = &nb_columns;
        args->client_sd = client_sd;
        args->stock = &stock;

        pthread_create(&client_thread, NULL, handle_client, (void *)args);
        pthread_detach(client_thread); // Évite les fuites mémoire
    }

    close_socket(&se);

    exit(EXIT_SUCCESS);
}
