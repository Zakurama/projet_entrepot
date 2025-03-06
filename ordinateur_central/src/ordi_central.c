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

void gestionnaire_inventaire(int se){
    char buffer_reception_ID_articles[MAXOCTETS];
    char buffer_reception_pos_articles[MAXOCTETS];
    // int ID_articles[MAX_ARTICLES_LISTE_ATTENTE];
    // int positions_possibles_articles[MAX_ARTICLES_LISTE_ATTENTE][MAX_ESPACE_STOCK];
    // int positions_choisies_articles[MAX_ARTICLES_LISTE_ATTENTE];
    int ID_robot = 0;
    listen_to(se);
    int client_sd = accept_client(se);
    while (1){

        // On attend une commande de l'inventaire
        recev_message(client_sd, buffer_reception_ID_articles); // la liste des articles (ID)
        recev_message(client_sd, buffer_reception_pos_articles); // la liste des positions

        // On extrait les articles demandés et leurs positions
        char *item_names_requested[MAX_ARTICLES_LISTE_ATTENTE];

        for (int i = 0; i < MAX_ARTICLES_LISTE_ATTENTE; i++) {
            item_names_requested[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
        }

        int L_n_requested[MAX_ARTICLES_LISTE_ATTENTE];
        int count;
        char *error = parse_client_request(buffer_reception_ID_articles, MAX_ARTICLES_LISTE_ATTENTE, L_n_requested, item_names_requested, &count);
        if (error != NULL) {
            fprintf(stderr, "Error in parse client request: %s\n", error);
            continue;
        }

        int *L_n_stock[MAX_ARTICLES_LISTE_ATTENTE]; // liste des valeurs sur une position par article
        int *L_x_stock[MAX_ARTICLES_LISTE_ATTENTE]; // liste des allées par article
        int *L_y_stock[MAX_ARTICLES_LISTE_ATTENTE]; // liste des bacs par article
        char *item_names_stock[MAX_ARTICLES_LISTE_ATTENTE];

        for (int i = 0; i < MAX_ARTICLES_LISTE_ATTENTE; i++) {
            L_n_stock[i] = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int));
            L_x_stock[i] = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int));
            L_y_stock[i] = malloc(MAX_ARTICLES_LISTE_ATTENTE * sizeof(int));
            item_names_stock[i] = malloc(MAX_ITEMS_NAME_SIZE * sizeof(char));
        }

        int count_stock[MAX_ARTICLES_LISTE_ATTENTE]; // nombre de positions par article
        int nb_items;
        char *response = parse_stock(buffer_reception_pos_articles, MAX_ARTICLES_LISTE_ATTENTE, L_n_stock, L_x_stock, L_y_stock, item_names_stock, count_stock, &nb_items);
        if (response!=NULL){
            fprintf(stderr, "Error in parse stock request: %s\n", response);
            continue;
        }

        printf("Parsing successful\n");
        printf("buffer_reception_ID_articles: %s\n", buffer_reception_ID_articles);
        printf("buffer_reception_pos_articles: %s\n", buffer_reception_pos_articles);
                
        // Choisir les articles dans les stocks
        // Liste des articles sélectionnés
        SelectedItem selected_items[MAX_ARTICLES_LISTE_ATTENTE];
        int nb_selected = choose_items_stocks(item_names_requested, L_n_requested, count_requested,item_names_stock, L_n_stock, L_x_stock, L_y_stock, count_stock,selected_items);

        // On choisit le robot qui traitera la tâche et la position de l'article souhaité en stock
        for(int i = 0; i < nb_selected; i++){
            ID_robot = (ID_robot+1)%NB_ROBOT; // Pour l'instant pas de choix optimal du robot on prends juste à son tour les robots
            // On met à jour la liste des articles et la liste de position du robot
            // TODO (memoire partagée)
        }
        // On informe l'inventaire qu'on a bien pris en compte sa demande (on indique quels articles de l'inventaire vont être pris)
        // TODO

        // Free allocated memory
        for (int i = 0; i < MAX_ARTICLES_LISTE_ATTENTE; i++) {
            free(item_names_requested[i]);
            free(L_n_stock[i]);
            free(L_x_stock[i]);
            free(L_y_stock[i]);
            free(item_names_stock[i]);
        }
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
