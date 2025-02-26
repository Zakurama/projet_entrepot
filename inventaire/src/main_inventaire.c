#include "inventaire.h"

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

    pthread_t manager_thread;
    thread_args_t *args = malloc(sizeof(thread_args_t));
    args->nb_rows = &nb_rows;
    args->nb_columns = &nb_columns;
    args->stock = &stock;
    pthread_create(&manager_thread, NULL, stock_manager, (void*)args);

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
