#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define CHECK_ERROR(val1,val2,msg)   if (val1==val2) \
                                    { perror(msg); \
                                        exit(EXIT_FAILURE); }

#define LOCALPORT   3000

#define MAXOCTETS   500
#define MAXCLIENTS  100

#define STOCK_INIT  5

pthread_mutex_t stock_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
    int client_sd;
    int * nb_rows;
    int * nb_columns;
    int *** stock;
} thread_args_t;

void init_stock(int ***stock, int nb_rows, int nb_columns);
void print_stock(int **stock, int nb_rows, int nb_columns);
void add_row(int ***stock, int *nb_rows, int nb_columns, int nb_supplementary_rows);
void add_column(int ***stock, int nb_rows, int *nb_columns, int nb_supplementary_columns);
char * handle_request(int ***stock, int nb_rows, int nb_columns, const char *request, int client);
char * parse_message(const char *request, int *L_n, int *L_x, int *L_y, int *count, int max_elements);
char * modify_stock(int ***stock, int nb_rows, int nb_columns, int *rows, int *columns, int *values, int count);
void free_stock(int **stock, int nb_rows);
void init_tcp_socket(int *sd, char *local_ip, u_int16_t local_port);
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

void init_tcp_socket(int *sd, char *local_ip, u_int16_t local_port){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_ERROR(*sd, -1, "Erreur socket non cree !!! \n");
    
    //preparation de l'adresse de la socket
    addr.sin_family = AF_INET;
    addr.sin_port = htons(local_port);
    addr.sin_addr.s_addr = inet_addr(local_ip);
    
    //Affectation d'une adresse a la socket
    int erreur = bind(*sd, (const struct sockaddr *) &addr, addr_len);
    CHECK_ERROR(erreur, -1, "Erreur de bind !!! \n");
}

void *handle_client(void *arg) {
    
    thread_args_t *args = (thread_args_t *)arg;
    int client_sd = args->client_sd;
    int *nb_rows = args->nb_rows;  // Récupère le pointeur vers nb_rows
    int *nb_columns = args->nb_columns;  // Récupère le pointeur vers nb_columns
    int ***stock = args->stock;  // Récupère le pointeur vers stock
    free(args);  // Libère la mémoire de la structure allouée

    char buff_reception[MAXOCTETS + 1];
    int nb_car;
    char buff_emission[MAXOCTETS + 1];

    while (1) {
        // Réception du message de la part du lecteur
        nb_car = recv(client_sd, buff_reception, MAXOCTETS, 0);
        CHECK_ERROR(nb_car, -1, "\nProblème de réception !!!\n");
        if (nb_car == 0) { // Si le client se déconnecte
            break;
        }
        buff_reception[nb_car] = '\0';

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

        nb_car = send(client_sd, buff_emission, strlen(buff_emission) + 1, 0);
        CHECK_ERROR(nb_car, -1, "\nProblème de réception !!!\n");
        if (nb_car == 0) { // Si le client se déconnecte
            break;
        }
    }

    CHECK_ERROR(close(client_sd), -1, "Erreur lors de la fermeture de la socket");
    return NULL;
}

void *stock_manager(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int *nb_rows = args->nb_rows;  // Récupère le pointeur vers nb_rows
    int *nb_columns = args->nb_columns;  // Récupère le pointeur vers nb_columns
    int ***stock = args->stock;  // Récupère le pointeur vers stock
    int client_sd = args->client_sd;
    free(args);  // Libère la mémoire de la structure allouée

    char command[MAXOCTETS];

    while (1) {
        printf("\n[GERANT] Entrez une commande (stock, add_row, add_column, add_stock, exit) : ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Supprime le '\n'

        if (strcmp(command, "exit") == 0) {
            printf("[GERANT] Fin du mode gérant.\n");
            pthread_exit(NULL);
        }
        else if (strcmp(command, "add_row") == 0) {
            int nb_supplementary_rows;

            printf("Combien de lignes supplémentaires ? "); 

            fgets(command, sizeof(command), stdin);
            if (sscanf(command, "%d", &nb_supplementary_rows) == 1) {
                add_row(stock, nb_rows, *nb_columns, nb_supplementary_rows);
                printf("[Réponse] Ligne ajoutée avec succès !\n");
            } else {
                printf("[Réponse] Entrée invalide ! Veuillez entrer un nombre.\n");
            }
        } 
        else if (strcmp(command, "add_column") == 0) {
            int nb_supplementary_columns;
            printf("Combien de colonnes supplémentaires ? ");

            fgets(command, sizeof(command), stdin);
            if (sscanf(command, "%d", &nb_supplementary_columns) == 1) {
                add_column(stock, *nb_rows, nb_columns, nb_supplementary_columns);
                printf("[Réponse] Colonne ajoutée avec succès !\n");
            } else {
                printf("[Réponse] Entrée invalide ! Veuillez entrer un nombre.\n");
            }
           
        }
        else if (strcmp(command, "stock") == 0) {
            printf("[Réponse] Stock :\n");
            print_stock(*stock, *nb_rows, *nb_columns);
        }
        else if(strcmp(command, "add_stock")==0){
            printf("Enter the number_row.column (N_X.Y,...) :");
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0; // Supprime le '\n'
            pthread_mutex_lock(&stock_mutex);  // Verrouille l'accès au stock
            char *response = handle_request(stock, *nb_rows, *nb_columns, command, client_sd );
            pthread_mutex_unlock(&stock_mutex); // Déverrouille

            if (response != NULL) {
                printf("[Réponse] %s\n", response);
            } else {
                printf("[Réponse] Stock mis à jour avec succès !\n");
            }
        }
        else{
            printf("Commande inconnue. Essayez : stock, add_row, add_column, add_stock, exit.\n");
        }
        printf("DEBUG: Nouvelle taille - nb_rows = %d, nb_columns = %d\n", *nb_rows, *nb_columns);

    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int nb_columns = 5;
    int nb_rows = 5;
    int **stock;

    init_stock(&stock, nb_rows, nb_columns);

    int se;

    if (argc == 2){
        init_tcp_socket(&se, argv[1], LOCALPORT);
    } 
    else if (argc == 3){ 
        init_tcp_socket(&se, argv[1], (u_int16_t) atoi(argv[2]));
    }
    else {
        fprintf(stderr, "Usage: %s <local_ip> [<local_port>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Communication avec les clients

    CHECK_ERROR(listen(se, MAXCLIENTS), -1, "Erreur de listen !!!\n");

    struct sockaddr_in adrclient;
    socklen_t adrclient_len = sizeof(adrclient);

    pthread_t manager_thread;
    thread_args_t *args = malloc(sizeof(thread_args_t));
    args->nb_rows = &nb_rows;
    args->nb_columns = &nb_columns;
    args->client_sd = 0;  
    args->stock = &stock;
    pthread_create(&manager_thread, NULL, stock_manager, (void*)args);

    while (1) {
        int client_sd = accept(se, (struct sockaddr *)&adrclient, &adrclient_len);
        CHECK_ERROR(client_sd, -1, "Erreur de accept !!!\n");

        pthread_t client_thread;
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->nb_rows = &nb_rows;
        args->nb_columns = &nb_columns;
        args->client_sd = client_sd;
        args->stock = &stock;

        pthread_create(&client_thread, NULL, handle_client, (void *)args);
        pthread_detach(client_thread); // Évite les fuites mémoire
    }

    close(se);

    exit(EXIT_SUCCESS);
}
