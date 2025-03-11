#include "inventaire.h"

int main(int argc, char *argv[]) {
    int nb_columns = 5;
    int nb_rows = 5;
    const char *item_placement = "5_1.1,5_2.2";
    int nb_items = 2;
    const int max_item_name_size= 500;

    // Allocate and initialize items
    item_t *items = malloc(nb_items * sizeof(item_t));
    for (int i = 0; i < 2; i++) {
        items[i].stock = NULL; // Ensure stock is initialized
        init_stock(&items[i], nb_rows, nb_columns, item_placement);
        items[i].name = malloc(max_item_name_size * sizeof(char));
        snprintf(items[i].name, max_item_name_size, "item%d", i);
    }

    int se;
    int client_sd;
    int computer_sd;

    if (argc == 2){
        init_tcp_socket(&se, argv[1], LOCALPORT,1);
        init_tcp_socket(&computer_sd, REMOTEIP, REMOTEPORT,0);
    } 
    else if (argc == 3){ 
        init_tcp_socket(&se, argv[1], (u_int16_t) atoi(argv[2]),1);
        init_tcp_socket(&computer_sd, REMOTEIP, REMOTEPORT,0);
    }
    else if (argc ==4){
        init_tcp_socket(&se, argv[1], (u_int16_t) atoi(argv[2]),1);
        init_tcp_socket(&computer_sd, argv[3], REMOTEPORT,0);
    }
    else if (argc==5){
        init_tcp_socket(&se, argv[1], (u_int16_t) atoi(argv[2]),1);
        init_tcp_socket(&computer_sd, argv[3], (u_int16_t) atoi(argv[4]),0);
    }
    else {
        fprintf(stderr, "Usage: %s <local_ip> [<local_port> <remote_ip> <remote_port>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char message[50];
    snprintf(message, sizeof(message), "rows,%d", nb_rows);
    send_message(computer_sd, message);
    recev_message(computer_sd, message);
    // Check if the message is valid
    if (strcmp(message, "Size updated successfully") != 0) {
        fprintf(stderr, "Failed to update the number of rows\n");
        exit(EXIT_FAILURE);
    }

    snprintf(message, sizeof(message), "columns,%d", nb_columns);
    send_message(computer_sd, message);
    recev_message(computer_sd, message);
    // Check if the message is valid
    if (strcmp(message, "Size updated successfully") != 0) {
        fprintf(stderr, "Failed to update the number of columns\n");
        exit(EXIT_FAILURE);
    }



    // Communication avec les clients

    listen_to(se);

    pthread_t manager_thread;
    thread_args_t *args = malloc(sizeof(thread_args_t));
    args->nb_rows = &nb_rows;
    args->nb_columns = &nb_columns;
    args->items = &items;
    args->nb_items = &nb_items;
    args->computer_sd = computer_sd;
    pthread_create(&manager_thread, NULL, stock_manager, (void*)args);
    pthread_detach(manager_thread); // Évite les fuites mémoire

    while (1) {
        client_sd = accept_client(se);
        
        pthread_t client_thread;
        thread_args_t *args = malloc(sizeof(thread_args_t));
        args->nb_rows = &nb_rows;
        args->nb_columns = &nb_columns;
        args->client_sd = client_sd;
        args->items = &items;
        args->nb_items = &nb_items;
        args->computer_sd = computer_sd;

        pthread_create(&client_thread, NULL, handle_client, (void *)args);
        pthread_detach(client_thread); // Évite les fuites mémoire
    }

    close_socket(&se);

    exit(EXIT_SUCCESS);
}
