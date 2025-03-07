#include "ordi_central.h"
#include "utils.h"
#include "tcp.h"
#include "inventaire.h"

void trajectoire(const char* pos_initiale, const char* pos_finale, char path[MAX_WAYPOINTS][SIZE_POS]){

    int pos_index = atoi(pos_initiale+1);
    char type_pos = pos_initiale[0];

    int pos_finale_index = atoi(pos_finale + 1);    
    char type_pos_finale = pos_finale[0];

    // On vérifie que les positions données sont cohérentes
    if(!(((type_pos=='S') && (pos_index%(NB_COLONNES + 1) != 0) && (pos_index % (2*(NB_COLONNES + 1))<(NB_COLONNES + 1)) ) || (type_pos!='S' && pos_index % (NB_COLONNES + 1) == 0))){
        printf("La position initiale donnée est non cohérente. Si de type S : ne doit pas être mutliple de %d, sinon doit être mutliple de %d\nSi de type S, il faut aussi que pos_initiale_index mod %d<%d\n",NB_COLONNES + 1,NB_COLONNES + 1,2*(NB_COLONNES + 1),NB_COLONNES + 1);
        exit(EXIT_FAILURE);
    }
    if(!(((type_pos_finale=='S') && (pos_finale_index%(NB_COLONNES + 1) != 0) && (pos_finale_index % (2*(NB_COLONNES + 1))<(NB_COLONNES + 1)) ) || (type_pos_finale!='S' && pos_finale_index%(NB_COLONNES + 1) == 0))){
        printf("La position finale donnée est non cohérente. Si de type S : ne doit pas être mutliple de %d, sinon doit être mutliple de %d\nSi de type S, il faut aussi que pos_finale_index mod %d<%d\n",NB_COLONNES + 1,NB_COLONNES + 1,2*(NB_COLONNES + 1),NB_COLONNES + 1);
        exit(EXIT_FAILURE);
    }            
    
    strcpy(path[0], pos_initiale);
    int i = 1;

    if(type_pos=='B'){
        // Si le début c'est un bac alors on retourne en PXX avant de repartir
        if(pos_index == 5){
            // On s'avance sur le deuxième bac
            strcpy(path[i], "B10");
            i++;
        }
        strcpy(path[i], "P25");
        i++;
        type_pos = 'P';
        pos_index = 25;
    }
    
    while (pos_index!=pos_finale_index || type_pos!=type_pos_finale){
        char holder_name_place[20];
        
        if(type_pos == 'P'){
            // On est au parking
            // On doit avancer sur la colone de montée
            sprintf(holder_name_place, "D%d", pos_index);
            strcpy(path[i], holder_name_place);
        }

        else if (pos_index > pos_finale_index){
            // On regarde ou on est
            if (type_pos == 'D'){
                // On est sur la colone de descente
                // On doit descendre sauf si on est sur la dernière place
                if(pos_index == (NB_COLONNES+1)*(2*NB_LIGNES)){
                    // On est sur la dernière place, on peut aller sur la colonne de montée
                    sprintf(holder_name_place, "M%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "D%d", pos_index + (NB_COLONNES + 1));
                    strcpy(path[i], holder_name_place);
                }

                
            }
           else if (type_pos == 'M'){
                // On est sur la colone de montée
                // On peut monter
                if(pos_index + NB_COLONNES > pos_finale_index){
                    sprintf(holder_name_place, "M%d", pos_index-(NB_COLONNES + 1));
                    strcpy(path[i], holder_name_place);
                }
            }
            else if (type_pos == 'S'){
                // On va vers la gauche
                if((pos_index)%(NB_COLONNES + 1)==0){
                    // On est arrivé en bout (début) des étagères
                    sprintf(holder_name_place, "M%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "S%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
            }
        }
                
        else if(pos_index +  NB_COLONNES >= pos_finale_index){
            // Je suis a la bonne altitude (on est devant la bonne étagère de stocks)
            if(pos_index < pos_finale_index && ((type_pos=='M')||(type_pos=='S'))){
                // On va vers la droite
                sprintf(holder_name_place, "S%d", pos_index+1);
                strcpy(path[i], holder_name_place);
            }
            // Je suis a la bonne altitude (on est devant le bon étage M)
            if(pos_index < pos_finale_index && ((type_pos=='D'))){
                // On va sur la colone de montée
                sprintf(holder_name_place, "M%d", pos_index);
                strcpy(path[i], holder_name_place);
            }
        }

        else if (pos_index < pos_finale_index){
            // On regarde ou on est
            if (type_pos == 'D'){
                // On est sur la colone de descente 
                // On peut descendre
                if(pos_index + NB_COLONNES +1<= pos_finale_index){
                    sprintf(holder_name_place, "D%d", pos_index+(NB_COLONNES + 1));
                    strcpy(path[i], holder_name_place);
                }
            }
            else if (type_pos == 'M'){
                // On est sur la colone de montée
                // On est sur la colone de descente
                // On doit monter sauf si on est sur la première place
                if(pos_index ==(NB_COLONNES + 1)){
                    // On est sur la première place, on peut aller sur la colonne de descente
                    sprintf(holder_name_place, "D%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "M%d", pos_index - (NB_COLONNES + 1));
                    strcpy(path[i], holder_name_place);
                }

            }
            else if (type_pos == 'S'){
                // On va vers la gauche
                if((pos_index)%(NB_COLONNES + 1)==0){
                    // On est arrivé en bout (début) des étagères
                    sprintf(holder_name_place, "M%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "S%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
            }

        }
        if(pos_index == pos_finale_index){
            if((type_pos=='D') && (type_pos_finale=='P')){
                // Je suis au bon niveau mais je veux aller au parking
                // On se déplace
                sprintf(holder_name_place, "P%d" ,pos_index);
                strcpy(path[i], holder_name_place);
            }
            if((type_pos=='M') && (type_pos_finale=='B')){
                // Je suis au bon niveau mais je veux aller sur les bacs
                // On se déplace
                sprintf(holder_name_place, "D%d" ,pos_index);
                strcpy(path[i], holder_name_place);
            }
            if((type_pos=='D') && (type_pos_finale=='B')){
                // Je suis au bon niveau mais je veux aller aux bacs
                // On se déplace
                sprintf(holder_name_place, "B%d" ,pos_index);
                strcpy(path[i], holder_name_place);
            }
        }

        pos_index = atoi(path[i]+1);
        type_pos = path[i][0];
        i++;
    }
}

char* convert_request_strings_to_lists(char *buffer_reception_ID_articles, char *buffer_reception_pos_articles,char *item_names_requested[MAX_ARTICLES_LISTE_ATTENTE],int L_n_requested[MAX_ARTICLES_LISTE_ATTENTE],int *L_n_stock[MAX_ARTICLES_LISTE_ATTENTE], int *L_x_stock[MAX_ARTICLES_LISTE_ATTENTE], int *L_y_stock[MAX_ARTICLES_LISTE_ATTENTE], char *item_names_stock[MAX_ARTICLES_LISTE_ATTENTE], int* count_requested, int count_stock[MAX_ARTICLES_LISTE_ATTENTE],int* nb_items) {

    for (int i = 0; i < MAX_ARTICLES_LISTE_ATTENTE; i++) {
        item_names_requested[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    for (int i = 0; i < MAX_ARTICLES_LISTE_ATTENTE; i++) {
        L_n_stock[i] = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int));
        L_x_stock[i] = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int));
        L_y_stock[i] = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int));
        item_names_stock[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
    }

    // On extrait les articles demandés et leurs positions
    char *error = parse_client_request(buffer_reception_ID_articles, MAX_ARTICLES_LISTE_ATTENTE, L_n_requested, item_names_requested,count_requested);
    if (error != NULL) {
        return error;
    }

    char *response = parse_stock(buffer_reception_pos_articles, MAX_ARTICLES_LISTE_ATTENTE, L_n_stock, L_x_stock, L_y_stock, item_names_stock, count_stock, nb_items);
    if (response != NULL) {
        return error;
    }
    return NULL;
}

void update_shared_memory_stock(Robot *robot, Item_selected selected_item,int index_pos){
    // Trouver le prochain indice disponible pour ajouter un nouvel article
    int idx = 0;
    while (robot->item_name[idx] != NULL && idx < MAX_WAYPOINTS) {
        idx++; // Trouver la prochaine place vide
    }
    
    // Ajouter le nouvel Item_selected
    if (idx < MAX_WAYPOINTS) {
        // Ajouter le nom de l'article
        robot->item_name[idx] = selected_item.item_name;

        // Ajouter les positions (copier les coordonnées du stock)
        robot->positions[idx] = (int *)malloc(2 * sizeof(int)); // Allouer de la mémoire pour [x, y]
        if (robot->positions[idx] != NULL) {
            robot->positions[idx][0] = selected_item.positions[index_pos][0] + 1; // x, +1 car premier bac (1,1) et non (0,0)
            robot->positions[idx][1] = selected_item.positions[index_pos][1] + 1; // y
        }

        // Ajouter les quantités
        robot->quantities[idx] = (int *)malloc(sizeof(int)); // Allouer de la mémoire pour la quantité
        if (robot->quantities[idx] != NULL) {
            robot->quantities[idx][0] = selected_item.quantities[index_pos];
        }
    }
}

int choose_items_stocks(char *item_names_requested[], int L_n_requested[], int count_requested,char *item_names_stock[], int *L_n_stock[], int *L_x_stock[], int *L_y_stock[], int count_stock[],Item_selected selected_items[]) {
    
    // Choix fait de manière non optimisé : on prend les premiers articles que l'on trouve en parcourant le stock dans l'ordre

    int total_selected = 0;

    for (int i = 0; i < count_requested; i++) {
        int needed = L_n_requested[i];
        selected_items[i].item_name = item_names_requested[i];
        selected_items[i].positions = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int *));
        selected_items[i].quantities = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int));
        selected_items[i].count = 0;

        // Chercher l'article dans le stock
        for (int j = 0; j < MAX_ARTICLES_LISTE_ATTENTE; j++) {
            if (!(item_names_stock[j] != NULL && strcmp(item_names_requested[i], item_names_stock[j]) == 0)) {
                continue;
            }
            // Sélectionner les positions disponibles
            for (int k = 0; k < count_stock[j] && needed > 0; k++) {
                int available = L_n_stock[j][k];


                if (available <= 0) {
                    continue;
                }

                // Stocker les coordonnées [x, y]
                selected_items[i].positions[selected_items[i].count] = malloc(2 * sizeof(int));
                selected_items[i].positions[selected_items[i].count][0] = L_x_stock[j][k]; // x
                selected_items[i].positions[selected_items[i].count][1] = L_y_stock[j][k]; // y

                // Déterminer la quantité à prélever
                int taken = (needed < available) ? needed : available;
                selected_items[i].quantities[selected_items[i].count] = taken;
                selected_items[i].count++;

                needed -= taken;
            }
        }
        if (selected_items[i].count > 0) {
            total_selected++;
        }
    }

    return total_selected;
}

void print_robot_state(Robot* robot){
    printf("Robot ID: %d\n", robot->ID);
    for (int i = 0; i < MAX_WAYPOINTS; i++) {
        if (robot->waypoints[i] != 0)
            printf("  - Waypoint: %d\n", robot->waypoints[i]);
        if (robot->item_name[i] != NULL)
            printf("  - Item: %s\n", robot->item_name[i]);
        if (robot->positions[i] != NULL)
            printf("  - Position: (%d, %d)\n", robot->positions[i][0], robot->positions[i][1]);
        if (robot->quantities[i] != NULL)
            printf("  - Quantity: %d\n", *robot->quantities[i]);
    }
}

void convert_items_to_lists(Item_selected *selected_items, int num_items,char *chosen_item_names[MAX_ARTICLES_LISTE_ATTENTE],int *chosen_x_positions[MAX_ARTICLES_LISTE_ATTENTE],int *chosen_y_positions[MAX_ARTICLES_LISTE_ATTENTE],int *chosen_quantities[MAX_ARTICLES_LISTE_ATTENTE],int chosen_counts[MAX_ARTICLES_LISTE_ATTENTE]) {
    for (int i = 0; i < num_items; i++) {
        Item_selected item = selected_items[i];

        // Copier le nom de l'article
        chosen_item_names[i] = item.item_name;

        // Allouer et copier les positions (x, y)
        chosen_x_positions[i] = (int *)malloc(item.count * sizeof(int));
        chosen_y_positions[i] = (int *)malloc(item.count * sizeof(int));
        for (int j = 0; j < item.count; j++) {
            chosen_x_positions[i][j] = item.positions[j][0] + 1; // x
            chosen_y_positions[i][j] = item.positions[j][1] + 1; // y
        }

        // Allouer et copier les quantités
        chosen_quantities[i] = (int *)malloc(item.count * sizeof(int));
        for (int j = 0; j < item.count; j++) {
            chosen_quantities[i][j] = item.quantities[j];
        }

        // Copier le count
        chosen_counts[i] = item.count;
    }
    }


// message format: "itemName1;N_X.Y,N_X.Y,.../itemName2;N_X.Y,..
char *parse_stock(const char *request, int max_elements, int *L_n[max_elements], int *L_x[max_elements], int *L_y[max_elements], char *item_names[max_elements], int count[max_elements], int *nb_items_request){
    char temp[strlen(request) + 1];
    strcpy(temp, request); // Create a modifiable copy of the string

    *nb_items_request = 0;
    char *item_token = strtok(temp, "/");
    if (item_token == NULL){
        return "Invalid request format\n";
    }
    while (item_token != NULL){
        char item_token_copy[strlen(item_token) + 1];
        strcpy(item_token_copy, item_token);
        char *name = strtok(item_token_copy, ";");
        char *positions = strtok(NULL, ";");

        if (name == NULL || positions == NULL){
            return "Invalid request format\n";
        }
        if (*nb_items_request >= max_elements) {
            return "Too many items requested\n";
        }

        strcpy(item_names[*nb_items_request], name);
        
        char *return_parse_message = parse_message(positions, L_n[*nb_items_request], L_x[*nb_items_request], L_y[*nb_items_request], &count[*nb_items_request], max_elements);
        if (return_parse_message != NULL){
            return return_parse_message;
        }

        (*nb_items_request)++;
        strcpy(temp, request);
        item_token = strtok(temp, "/");
        for (int i = 0; i < *nb_items_request; i++){
            item_token = strtok(NULL, "/");
        }
    }
    for (int i=0; i<*nb_items_request; i++){
        for (int j=0; j<count[i]; j++){
            L_x[i][j] -= 1;
            L_y[i][j] -= 1;
        }
    }
    return NULL;
}

/* Return a string in the format: "itemName1;N_X.Y,N_X.Y,.../itemName2;N_X.Y,..."
 */
char *create_inventory_string(int nb_items, int max_elements, int count[max_elements], int *L_n[max_elements], int *L_x[max_elements], int *L_y[max_elements], char *item_names[max_elements]){
    char *inventory_string = malloc(MAXOCTETS * sizeof(char));
    CHECK_ERROR(inventory_string, NULL, "Failed to allocate memory for inventory string");

    inventory_string[0] = '\0';
    for (int i = 0; i < nb_items; i++){
        char item_string[MAXOCTETS];
        item_string[0] = '\0';
        for (int j = 0; j < count[i]; j++){
            char position_string[MAXOCTETS];
            sprintf(position_string, "%d_%d.%d", L_n[i][j], L_x[i][j], L_y[i][j]);
            strcat(item_string, position_string);
            if (j < count[i] - 1){
                strcat(item_string, ",");
            }
        }
        strcat(inventory_string, item_names[i]);
        strcat(inventory_string, ";");
        strcat(inventory_string, item_string);
        if (i < nb_items - 1){
            strcat(inventory_string, "/");
        }
    }
    return inventory_string;
}

