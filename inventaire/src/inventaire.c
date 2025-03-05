#include "inventaire.h"

pthread_mutex_t stock_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_stock(item_t *item, int nb_rows, int nb_columns, const char *item_placement) {
    item->quantity = 0;
    int max_elements = 50;
    int L_n[max_elements], L_x[max_elements], L_y[max_elements], count;

    // Parse the input message
    parse_message(item_placement, L_n, L_x, L_y, &count, max_elements);
    
    // Humans start counting from 1
    for (int i = 0; i < count; i++) {
        L_x[i]--;
        L_y[i]--;
    }

    // Allocate memory correctly
    item->stock = (int **)malloc(nb_rows * sizeof(int *));
    CHECK_ERROR(item->stock, NULL, "Failed to allocate memory for rows");
    for (int i = 0; i < nb_rows; i++) {
        item->stock[i] = (int *)malloc(nb_columns * sizeof(int));
        CHECK_ERROR(item->stock[i], NULL, "Failed to allocate memory for columns");
    }

    // Initialize stock with 0
    for (int i = 0; i < nb_rows; i++) {
        for (int j = 0; j < nb_columns; j++) {
            item->stock[i][j] = 0;
        }
    }

    // Place the stock items at the correct positions
    for (int i = 0; i < count; i++) {
        if (L_x[i] >= 0 && L_x[i] < nb_rows && L_y[i] >= 0 && L_y[i] < nb_columns) {
            item->stock[L_x[i]][L_y[i]] = L_n[i];
            item->quantity += L_n[i];
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

void add_row(item_t *items, int nb_items, int *nb_rows, int nb_columns, int nb_supplementary_rows) {
    for (int k = 0; k < nb_items; k++) {
        // Reallocate stock array for each item 
        items[k].stock = (int **)realloc(items[k].stock, (*nb_rows + nb_supplementary_rows) * sizeof(int *));
        CHECK_ERROR(items[k].stock, NULL, "Failed to allocate memory for new rows");

        // Allocate and initialize new rows
        for (int i = 0; i < nb_supplementary_rows; i++) {
            items[k].stock[*nb_rows + i] = (int *)malloc(nb_columns * sizeof(int));
            CHECK_ERROR(items[k].stock[*nb_rows + i], NULL, "Failed to allocate memory for new row");

            for (int j = 0; j < nb_columns; j++) {
                items[k].stock[*nb_rows + i][j] = NEW_STOCK_INIT_VALUE;
            }
        }
    }
    
    // Update the number of rows
    (*nb_rows) += nb_supplementary_rows;
}

void add_column(item_t *items, int nb_items, int nb_rows, int *nb_columns, int nb_supplementary_columns) {
    for (int k = 0; k < nb_items; k++) {
        for (int i = 0; i < nb_rows; i++) {
            // Reallocate each row to accommodate the new columns
            items[k].stock[i] = (int *)realloc(items[k].stock[i], (*nb_columns + nb_supplementary_columns) * sizeof(int));
            CHECK_ERROR(items[k].stock[i], NULL, "Failed to allocate memory for new columns");

            // Initialize new columns with NEW_STOCK_INIT_VALUE
            for (int j = *nb_columns; j < *nb_columns + nb_supplementary_columns; j++) {
                items[k].stock[i][j] = NEW_STOCK_INIT_VALUE;
            }
        }
    }

    // Update column count
    (*nb_columns) += nb_supplementary_columns;
}

// message format: "itemName_N,itemName_N,..."
char *check_client_request(const char *request, item_t *items, int nb_items, int max_elements) {
    int count = 0;
    char name[MAX_ITEMS_NAME_SIZE];
    int value;
    char *item_names[max_elements];

    char temp[strlen(request) + 1];
    strcpy(temp, request); // Create a modifiable copy of the string

    char *token = strtok(temp, ",");
    while (token != NULL) {
        if (count >= max_elements) {
            return "Maximum number of elements exceeded, please reduce the number to change\n";
        }

        if (sscanf(token, "%[^_]_%d", name, &value) != 2) {
            return "Invalid request format\n";
        }
        int index = get_item_index(items, nb_items, name);
        if (index == -1) {
            return "Item not found\n";
        }
        if (value < 0) {
            return "Clients cannot add stock\n";
        }
        else if (items[index].quantity < value) {
            char *error_message = (char *)malloc(100 * sizeof(char));
            snprintf(error_message, 100, "Cannot take %d of stock %s at current value %d\n", value, name, items[index].quantity);
            return error_message;
        }

        // Check if name is already in item_names
        for (int i = 0; i < count; i++) {
            if (strcmp(item_names[i], name) == 0) {
                return "Duplicate item name in request\n";
            }
        }

        // Store the item name
        item_names[count] = strdup(name);
        count++;
        token = strtok(NULL, ",");
    }
    return NULL;
}

/* Parse the message and store the values in the arrays L_n, L_x, L_y
 * The message format is "N_X.Y,N_X.Y,..."
 * The function returns an error message if the message is not correctly formatted
 */
char *parse_message(const char *request, int *L_n, int *L_x, int *L_y, int *count, int max_elements) {
    *count = 0;  // Initialize count

    char temp[strlen(request) + 1];
    strcpy(temp, request); // Create a modifiable copy of the string

    char *token = strtok(temp, ",");
    while (token != NULL) {
        if (*count >= max_elements) {
            return "Maximum number of elements exceeded, please reduce the number to change\n";
        }
        if (sscanf(token, "%d_%d.%d", &L_n[*count], &L_x[*count], &L_y[*count]) != 3) {
            return "Invalid request format\n";
        }
        (*count)++;
        token = strtok(NULL, ",");
    }
    return NULL;
}

/* 
 * The message format is "itemName1;N_X.Y,N_X.Y,.../itemName2;N_X.Y,..."
 * The function returns an error message if the message is not correctly formatted
 * central_computer is a boolean that indicates if the request is coming from the central computer
 */
char *handle_items_request(item_t *items, int nb_items, int nb_rows, int nb_columns, const char *request, int central_computer) {
    int max_elements = 50;
    int item_index[max_elements];

    int *L_n[max_elements];
    int *L_x[max_elements];
    int *L_y[max_elements];
    int count[max_elements];

    for (int i = 0; i < max_elements; i++) {
        L_n[i] = malloc(max_elements * sizeof(int));
        L_x[i] = malloc(max_elements * sizeof(int));
        L_y[i] = malloc(max_elements * sizeof(int));
        if (!L_n[i] || !L_x[i] || !L_y[i]) {
            return "Memory allocation failed\n";
        }
    }

    int count_items = 0;
    char temp[strlen(request) + 1];
    strcpy(temp, request);

    char *item_token = strtok(temp, "/");
    while (item_token != NULL) {
        char item_token_copy[strlen(item_token) + 1];
        strcpy(item_token_copy, item_token);
        char *name = strtok(item_token_copy, ";");
        char *positions = strtok(NULL, ";");

        if (name == NULL || positions == NULL) {
            // Free allocated memory
            for (int i = 0; i < max_elements; i++) {free(L_n[i]);free(L_x[i]);free(L_y[i]);}
            return "Invalid request format\n";
        }

        item_index[count_items] = get_item_index(items, nb_items, name);
        if (item_index[count_items] == -1) {
            // Free allocated memory
            for (int i = 0; i < max_elements; i++) {free(L_n[i]);free(L_x[i]);free(L_y[i]);}
            return "Item not found\n";
        }

        char *return_parse_message = parse_message(positions, L_n[count_items], L_x[count_items], L_y[count_items], &count[count_items], max_elements);
        if (return_parse_message != NULL) {
            // Free allocated memory
            for (int i = 0; i < max_elements; i++) {free(L_n[i]);free(L_x[i]);free(L_y[i]);}
            return return_parse_message;
        }

        count_items++;
        strcpy(temp, request);
        item_token = strtok(temp, "/");
        for (int i = 0; i < count_items; i++) {
            item_token = strtok(NULL, "/");
        }
    }

    for (int i = 0; i < count_items; i++) {
       
        // humans start counting from 1
        for (int j = 0; j < count[i]; j++) {
            L_x[i][j] -= 1;
            L_y[i][j] -= 1;
            if (L_x[i][j] < 0 || L_x[i][j] >= nb_rows || L_y[i][j] < 0 || L_y[i][j] >= nb_columns) {
                // Free allocated memory
                for (int i = 0; i < max_elements; i++) {free(L_n[i]);free(L_x[i]);free(L_y[i]);}
                return "Invalid row or column index\n";
            }
        }

    }
    for (int i = 0; i < count_items; i++) {
        if (central_computer){
            for (int j = 0; j < count[i]; j++) {
                L_n[i][j] = -L_n[i][j];
            }
        }
        char *return_message = modify_stock(&(items[item_index[i]]), nb_rows, nb_columns, L_x[i], L_y[i], L_n[i], count[i]);

        if (return_message != NULL) {
            // Free allocated memory
            for (int i = 0; i < max_elements; i++) {free(L_n[i]);free(L_x[i]);free(L_y[i]);}
            return return_message;
        }
        free(return_message);
    }

    // Free allocated memory
    for (int i = 0; i < max_elements; i++) {free(L_n[i]);free(L_x[i]);free(L_y[i]);}

    return NULL;
}

/* Return a string in the format: "itemName1;N_X.Y,N_X.Y,.../itemName2;N_X.Y,..."
 * The function returns an error message if the message is not correctly formatted
 */
char *transfer_stock(item_t *items, int nb_items, int nb_rows, int nb_columns, const char **item_names, int nb_items_request){
    char *result = malloc(1000 * sizeof(char));
    CHECK_ERROR(result, NULL, "Failed to allocate memory for result"); 
    result[0] = '\0'; // Initialize the result buffer

    for (int i = 0; i < nb_items_request; i++) {
        int index = get_item_index(items, nb_items, item_names[i]);
        if (index == -1) {
            free(result);
            return "Item not found\n";
        }
        else if (items[index].quantity == 0) {
            free(result);
            return "No stock available\n";
        }
        strcat(result, items[index].name);
        strcat(result, ";");
        for (int j = 0; j < nb_rows; j++) {
            for (int k = 0; k < nb_columns; k++) {
                if (items[index].stock[j][k] > 0) {
                    char temp[100];
                    snprintf(temp, 100, "%d_%d.%d,", items[index].stock[j][k], j+1, k+1); // humans start counting from 1
                    strcat(result, temp);
                }
            }
        }
        result[strlen(result) - 1] = '/'; // Replace the last comma with a slash
    }
    result[strlen(result) - 1] = '\0'; // Remove the last slash
    return result;
}

/* 
 * The message format is "itemName1;N_X.Y,N_X.Y,.../itemName2;N_X.Y,..."
 * The function returns NULL if the message is not correctly formatted
 * else returns the items names
 */
char **parse_items_names(item_t *items, int nb_items, const char *request, int *nb_items_request) {
    int max_items = 50;
    if (request == NULL || *request == '\0') {  // Handle empty input
        return NULL;
    }

    char **item_names = malloc(max_items * sizeof(char *));
    CHECK_ERROR(item_names, NULL, "Failed to allocate memory for item names");

    char temp[strlen(request) + 1];
    strcpy(temp, request);

    char *token = strtok(temp, "/");
    int count = 0;
    while (token != NULL) {
        if (count >= max_items) {  // Prevent exceeding limit
            for (int i = 0; i < count; i++) {
                free(item_names[i]);
            }
            free(item_names);
            return NULL;
        }

        char name[50];
        if (sscanf(token, "%[^;]", name) != 1) {
            for (int i = 0; i < count; i++) {
                free(item_names[i]);
            }
            free(item_names);
            return NULL;
        }
        item_names[count] = strdup(name);
        if (item_names[count] == NULL) {
            for (int i = 0; i < count; i++) {
                free(item_names[i]);
            }
            free(item_names);
            return NULL;
        }
        count++;
        token = strtok(NULL, "/");
    }

    *nb_items_request = count;
    return item_names;
}

char *modify_stock(item_t *item, int nb_rows, int nb_columns, int *rows, int *columns, int *values, int count) {
    if (item->stock == NULL) {
        return "Stock is not initialized\n";
    }

    // Validate input indices
    for (int i = 0; i < count; i++) {
        if (rows[i] < 0 || rows[i] >= nb_rows || columns[i] < 0 || columns[i] >= nb_columns) {
            return "Invalid row or column index\n";
        }
        else if (item->stock[rows[i]][columns[i]] + values[i] < 0) {
            char *error_message = (char *)malloc(100 * sizeof(char));
            snprintf(error_message, 100, 
                     "Cannot add %d of stock at (%d, %d). Current value: %d\n", 
                     values[i], rows[i], columns[i], item->stock[rows[i]][columns[i]]);
            return error_message;
        }
    }

    // Modify the stock
    for (int i = 0; i < count; i++) {
        item->stock[rows[i]][columns[i]] += values[i];
        item->quantity += values[i];
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
    item_t ** items = args->items;  // Récupère le pointeur vers items
    int * nb_items = args->nb_items;  // Récupère le pointeur vers nb_items
    int computer_sd = args->computer_sd;
    free(args);  // Libère la mémoire de la structure allouée

    char buff_reception[MAXOCTETS + 1] = "";
    char buff_emission[MAXOCTETS + 1] = "";

    while (1) {
        // Réception du message de la part du lecteur
        int nb_car = recv(client_sd, buff_reception, MAXOCTETS, 0);
        CHECK_ERROR(nb_car, -1, "\nProblème de réception !!!\n");
        if (nb_car == 0) { // Si le serveur se déconnecte
            pthread_exit(NULL);
        }

        buff_reception[nb_car] = '\0';
        strcpy(buff_emission, "");

        if (strcmp(buff_reception, "stock") == 0) {
            for (int i = 0; i < *nb_items; i++) {
                char temp[100] = "";
                sprintf(temp, "%s: %d\n", (*items)[i].name, (*items)[i].quantity);
                strcat(buff_emission, temp);

            }
            send_message(client_sd,buff_emission);
            // strcpy(buff_emission, get_stock_string(*stock, *nb_rows, *nb_columns));
        }
        else {
            // pthread_mutex_lock(&stock_mutex);
            char * message = check_client_request(buff_reception, *items, *nb_items, 50);
            // pthread_mutex_unlock(&stock_mutex);

            if (message != NULL) {
                strcpy(buff_emission, message);
            }
            else {
                
                // temporary until the central computer is implemented
                //strcpy(buff_emission, "Sending request to central computer\n");

                send_message(computer_sd, buff_reception);
                
                int nb_items_request;
                char **item_names = parse_items_names(*items, *nb_items, buff_reception, &nb_items_request);
                strcpy(buff_emission, transfer_stock(*items, *nb_items, *nb_rows, *nb_columns,(const char **) item_names, nb_items_request));
                send_message(computer_sd ,buff_emission);

                recev_message(computer_sd, buff_reception);
                pthread_mutex_lock(&stock_mutex);
                char *response = handle_items_request(*items, *nb_items, *nb_rows, *nb_columns, buff_reception, 1);
                pthread_mutex_unlock(&stock_mutex);
                if (response != NULL) {
                    strcpy(buff_emission, response);
                } else {
                    strcpy(buff_emission, "Stock updated successfully\n");
                }

            }
            
        }
        send_message(client_sd,buff_emission);
    }
    close_socket(&client_sd);
    return NULL;
}

void *stock_manager(void *arg) {
    thread_args_t *args = (thread_args_t *)arg;
    int *nb_rows = args->nb_rows;  // Récupère le pointeur vers nb_rows
    int *nb_columns = args->nb_columns;  // Récupère le pointeur vers nb_columns
    item_t **items = args->items;  // Récupère le pointeur vers items
    int *nb_items = args->nb_items;  // Récupère le pointeur vers nb_items
    free(args);  // Libère la mémoire de la structure allouée

    char command[MAXOCTETS];

    while (1) {
        printf("-----------------------------------------------------------------"); 
        printf("\n[MANAGER]\nEnter a command : \n\t1.stock\n\t2.add_row\n\t3.add_column\n\t4.add_stock\n\t5.add_item\n\t6.exit\n> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0; // Supprime le '\n'

        if (strcmp(command, "1") == 0) {
            printf("[MANAGER] Items : ");
            for (int i = 0; i < *nb_items; i++) {
                printf("%s ", (*items)[i].name);
            }
            printf("\n");
            printf("[MANAGER] Which item :\n");
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0; // Supprime le '\n'
            int item_index = get_item_index(*items, *nb_items, command);
            if (item_index == -1) {
                printf("[Answer] Item not found\n");
            } else {
                printf("[Answer] Stock :\n");
                print_stock((*items)[item_index].stock, *nb_rows, *nb_columns);
            }
        }
        else if (strcmp(command, "2") == 0) {
            int nb_supplementary_rows;

            printf("How many additional rows?\n> "); 

            fgets(command, sizeof(command), stdin);
            if (sscanf(command, "%d", &nb_supplementary_rows) == 1) {
                pthread_mutex_lock(&stock_mutex);  // Verrouille l'accès au stock
                add_row(*items, *nb_items, nb_rows, *nb_columns, nb_supplementary_rows);
                pthread_mutex_unlock(&stock_mutex); // Déverrouille
                printf("[Answer]  Rows added successfully!\n");
            } else {
                printf("[Answer] Invalid input! Please enter a number.\n");
            }
        } 
        else if (strcmp(command, "3") == 0) {
            int nb_supplementary_columns;
            printf("How many additional columns?\n> ");

            fgets(command, sizeof(command), stdin);
            if (sscanf(command, "%d", &nb_supplementary_columns) == 1) {
                pthread_mutex_lock(&stock_mutex);  // Verrouille l'accès au stock
                add_column(*items, *nb_items, *nb_rows, nb_columns, nb_supplementary_columns);
                pthread_mutex_unlock(&stock_mutex); // Déverrouille
                printf("[Answer] Columns added successfully!\n");  
            } else {  
                printf("[Answer] Invalid input! Please enter a number.\n");  
            }
           
        }
        else if(strcmp(command, "4")==0){
            printf("Enter the itemName;number_row.column (itemName1;N_X.Y,N_X.Y,.../itemName2;N_X,Y,...)\n> ");
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0; // Supprime le '\n'
            printf("Command : %s\n", command);
            pthread_mutex_lock(&stock_mutex);  // Verrouille l'accès au stock
            char *response = handle_items_request(*items, *nb_items, *nb_rows, *nb_columns, command, 0);
            pthread_mutex_unlock(&stock_mutex); // Déverrouille

            if (response != NULL) {
                printf("[Answer] %s\n", response);
            } else {
                printf("[Answer] Stock updated successfully!\n");
            }
        }
        // need to implement how to add items
        else if(strcmp(command, "5")==0){
            item_t item;
            printf("Enter the item name\n> ");
            fgets(command, sizeof(command), stdin);
            command[strcspn(command, "\n")] = 0; // Supprime le '\n'
            item.name = (char *)malloc(strlen(command) + 1);
            CHECK_ERROR(item.name, NULL, "Failed to allocate memory for item name");
            strcpy(item.name, command);

            item.stock = NULL;
            item.quantity = 0;

            pthread_mutex_lock(&stock_mutex);  // Verrouille l'accès au stock
            add_item(items, nb_items, item, nb_columns, nb_rows);
            pthread_mutex_unlock(&stock_mutex); // Déverrouille
            printf("[Answer] Item added successfully!\n");
        }
        else if (strcmp(command, "6") == 0) {
            printf("[MANAGER] End of manager.\n");
            pthread_exit(NULL);
        }
        else{
            printf("Unknown command\n");
        }

    }
    return NULL;
}

void add_item(item_t **items, int *nb_items, item_t item, int *nb_columns, int *nb_rows) {
    *items = (item_t *)realloc(*items, (*nb_items + 1) * sizeof(item_t));
    CHECK_ERROR(*items, NULL, "Failed to allocate memory for new item");

    (*items)[*nb_items].name = (char *)malloc(strlen(item.name) + 1);
    CHECK_ERROR((*items)[*nb_items].name, NULL, "Failed to allocate memory for item name");
    
    item.stock = NULL;

    strcpy((*items)[*nb_items].name, item.name);
    (*items)[*nb_items].quantity = item.quantity;

    init_stock(&item, *nb_rows, *nb_columns, "0_1.1"); 
    (*items)[*nb_items].stock = item.stock;
    

    (*nb_items)++;
}

int get_item_index(item_t *items, int nb_items, const char *name) {
    for (int i = 0; i < nb_items; i++) {
        char error_message[100];
        snprintf(error_message, sizeof(error_message), "Item name is NULL at index %d\n", i);
        CHECK_ERROR(items[i].name, NULL, error_message);
        if (strcmp(items[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}
