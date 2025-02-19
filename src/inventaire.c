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
#define LOCALIP     "172.20.10.2"
#define RBCPORT     6666
#define RBCIP       "172.20.10.2"

#define MAXOCTETS   150
#define MAXCLIENTS  100

void init_stock(int ***stock, int nb_rows, int nb_columns);
void print_stock(int *stock[5], int nb_rows, int nb_columns);
void add_row(int ***stock, int *nb_rows, int nb_columns, int nb_supplementary_rows);
void add_column(int ***stock, int nb_rows, int *nb_columns, int nb_supplementary_columns);
char * handle_request(int ***stock, int nb_rows, int nb_columns, const char *request);
char * parse_message(const char *request, int *L_n, int *L_x, int *L_y, int *count, int max_elements);
char * modify_stock(int ***stock, int nb_rows, int nb_columns, int *rows, int *columns, int *values, int count);
void free_stock(int **stock, int nb_rows);
void init_tcp_socket(int *sd, char *local_ip, u_int16_t local_port);
void *handle_client(void *client_socket);

void init_stock(int ***stock, int nb_rows, int nb_columns) {
    *stock = (int **)malloc(nb_rows * sizeof(int *));
    CHECK_ERROR(*stock, NULL, "Failed to allocate memory for rows");

    for (int i = 0; i < nb_rows; i++) {
        (*stock)[i] = (int *)malloc(nb_columns * sizeof(int));
        CHECK_ERROR((*stock)[i], NULL, "Failed to allocate memory for columns");

        // Initialize elements to 0
        for (int j = 0; j < nb_columns; j++) {
            (*stock)[i][j] = 0;
        }
    }
}

void print_stock(int *stock[5], int nb_rows, int nb_columns){
    printf("\n");
    for (int j=0; j<nb_columns; j++){
        printf("\t%d", j+1);
    }
    printf("\n\n");
    for (int i=0; i<nb_rows; i++){
        printf("%d\t", i+1);
        for (int j=0; j<nb_columns; j++){
            printf("%d\t", stock[i][j]);
        }
        printf("\n");
    }
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

char * handle_request(int ***stock, int nb_rows, int nb_columns, const char *request){
    int max_elements = 50;
    int L_n[max_elements], L_x[max_elements], L_y[max_elements], count;
    char * return_parse_message = parse_message(request, L_n, L_x, L_y, &count, max_elements);
    if (return_parse_message != NULL) {
        return return_parse_message;
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
    
    *sd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_ERROR(*sd, -1, "Erreur socket non cree !!! \n");
    
    //preparation de l'adresse de la socket
    addr.sin_family = AF_INET;
    addr.sin_port = htons(local_port);
    addr.sin_addr.s_addr = inet_addr(local_ip);
    
    //Affectation d'une adresse a la socket
    int erreur = bind(*sd, (const struct sockaddr *) &addr, addr_len);
    CHECK_ERROR(erreur, -1, "Erreur de bind !!! \n");
    printf("Waiting for request...\n");
}

void *handle_client(void *client_socket) {
    
    int client_sd = *((int *)client_socket);
    free(client_socket);

    char buff_reception[MAXOCTETS + 1];
    int nbcar;
    char buff_emission[MAXOCTETS + 1];

    while (1) {
        // Réception du message de la part du lecteur
        nbcar = recv(client_sd, buff_reception, MAXOCTETS, 0);
        CHECK_ERROR(nbcar, -1, "\nProblème de réception !!!\n");
        buff_reception[nbcar] = '\0';
        printf("MSG RECU DU CLIENT : %s\n", buff_reception);

        //vérification de la demande de déconnexion
        if (strcmp(buff_emission, "exit") == 0 || strcmp(buff_reception, "exit") == 0){
            CHECK_ERROR(close(client_sd), -1, "Erreur lors de la fermeture de la socket");
            break;
        }

        printf("SERVEUR> ");
        fgets(buff_emission, MAXOCTETS, stdin); // Lecture au clavier
        buff_emission[strlen(buff_emission) - 1] = '\0';
        nbcar = send(client_sd, buff_emission, strlen(buff_emission) + 1, 0);
        CHECK_ERROR(nbcar, 0, "\nProblème d'émission !!!\n");

        // Vérification de la demande de déconnexion
        if (strcmp(buff_emission, "exit") == 0 || strcmp(buff_reception, "exit") == 0) {
            CHECK_ERROR(close(client_sd), -1, "Erreur lors de la fermeture de la socket");
            break;
        }
    }

    close(client_sd);
    return NULL;
}

int main(int argc, char *argv[]) {
    int nb_columns = 5;
    int nb_rows = 5;
    int **stock;

    init_stock(&stock, nb_rows, nb_columns);
    print_stock(stock, nb_rows, nb_columns);
    // add_row(&stock, &nb_rows, nb_columns, 2);
    // print_stock(stock, nb_rows, nb_columns);
    // add_column(&stock, nb_rows, &nb_columns, 2);
    // print_stock(stock, nb_rows, nb_columns);
    // free_stock(stock, nb_rows);

    char * request = "2_1.2,1_2.3,-1_3.4";
    char * message = handle_request(&stock, nb_rows, nb_columns, request);
    print_stock(stock, nb_rows, nb_columns);
    if (message != NULL) {
        printf("%s\n", message);
    }

    // int se ;
    // struct sockaddr_in adrserveur;
    // struct sockaddr_in adrclient;
    // socklen_t adrclient_len = sizeof(adrclient);
    // int erreur;
    // int nbcar;

    // se=socket(AF_INET, SOCK_STREAM, 0);
    // CHECK_ERROR(se, -1, "Erreur socket non cree !!! \n");
    // printf("N° de la socket : %d \n", se);
    
    // //preparation de l'adresse de la socket
    // adrserveur.sin_family = AF_INET;
    // if (argc==1) adrserveur.sin_port = htons(LOCALPORT);
    // else adrserveur.sin_port = htons(atoi(argv[1]));
    // adrserveur.sin_addr.s_addr = inet_addr(LOCALIP);
    
    // //Affectation d'une adresse a la socket
    // erreur = bind(se, (const struct sockaddr *)&adrserveur, sizeof(adrserveur));
    // CHECK_ERROR(erreur, -1, "Erreur de bind !!!\n");
    
    // listen(se, MAXCLIENTS);
    // printf("En attente de connexion ...\n");

    // while (1) {
    //     int client_sd = accept(se, (struct sockaddr *)&adrclient, &adrclient_len);
    //     printf("N° de la socket de dialogue : %d\n", client_sd);
    //     CHECK_ERROR(client_sd, -1, "Erreur de accept !!!\n");

    //     pthread_t client_thread;
    //     int *client_socket = (int *)malloc(sizeof(int));
    //     *client_socket = client_sd;
    //     pthread_create(&client_thread, NULL, handle_client, (void *)client_socket);
    // }

    // close(se);

    // exit(EXIT_SUCCESS);
}
