#include "ordi_central.h"
#include "utils.h"
#include "tcp.h"
#include "inventaire.h"

void trajectoire(const char* pos_initiale, const char* pos_finale, char path[MAX_WAYPOINTS][SIZE_POS], int nb_lignes, int nb_colonnes){

    int pos_index = atoi(pos_initiale+1);
    char type_pos = pos_initiale[0];

    int pos_finale_index = atoi(pos_finale + 1);    
    char type_pos_finale = pos_finale[0];

    char holder_name_place[SIZE_POS];

    // On vérifie que les positions données sont cohérentes
    if(!(((type_pos=='S') && (pos_index%(nb_colonnes + 1) != 0) && (pos_index % (2*(nb_colonnes + 1))<(nb_colonnes + 1)) ) || (type_pos!='S' && pos_index % (nb_colonnes + 1) == 0))){
        printf("La position initiale donnée est non cohérente. Si de type S : ne doit pas être mutliple de %d, sinon doit être mutliple de %d\nSi de type S, il faut aussi que pos_initiale_index mod %d<%d\n",nb_colonnes + 1,nb_colonnes + 1,2*(nb_colonnes + 1),nb_colonnes + 1);
        exit(EXIT_FAILURE);
    }
    if(!(((type_pos_finale=='S') && (pos_finale_index%(nb_colonnes + 1) != 0) && (pos_finale_index % (2*(nb_colonnes + 1))<(nb_colonnes + 1)) ) || (type_pos_finale!='S' && pos_finale_index%(nb_colonnes + 1) == 0))){
        printf("La position finale donnée est non cohérente. Si de type S : ne doit pas être mutliple de %d, sinon doit être mutliple de %d\nSi de type S, il faut aussi que pos_finale_index mod %d<%d\n",nb_colonnes + 1,nb_colonnes + 1,2*(nb_colonnes + 1),nb_colonnes + 1);
        exit(EXIT_FAILURE);
    }            
    
    strcpy(path[0], pos_initiale);
    int i = 1;

    if(type_pos=='B'){
        // Si on est dans le bac alors on ressort sur la colonne de descente
        sprintf(holder_name_place, "D%d", pos_index);
        strcpy(path[i], holder_name_place);
        i++;
        type_pos = 'D';
    }
    
    while (pos_index!=pos_finale_index || type_pos!=type_pos_finale){
        
        holder_name_place[0] = '\0';
        
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
                if(pos_index == (nb_colonnes+1)*(2*nb_lignes)){
                    // On est sur la dernière place, on peut aller sur la colonne de montée
                    sprintf(holder_name_place, "M%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "D%d", pos_index + (nb_colonnes + 1));
                    strcpy(path[i], holder_name_place);
                }
                
            }
           else if (type_pos == 'M'){
                // On est sur la colone de montée
                // On peut monter
                sprintf(holder_name_place, "M%d", pos_index-(nb_colonnes + 1));
                strcpy(path[i], holder_name_place);
            }
            else if (type_pos == 'S'){
                // On va vers la gauche
                if((pos_index-1)%(nb_colonnes + 1)==0){
                    // On est arrivé en bout (début) des étagères
                    sprintf(holder_name_place, "M%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "S%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
            }
        }
                
        else if(pos_index +  nb_colonnes >= pos_finale_index){
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
            if(pos_index < pos_finale_index && type_pos=='S' && type_pos_finale!='S' ){
                // On va vers la gauche
                if((pos_index-1)%(nb_colonnes + 1)==0){
                    // On est arrivé en bout (début) des étagères
                    sprintf(holder_name_place, "M%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "S%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
            }
        }

        else if (pos_index < pos_finale_index){
            // On regarde ou on est
            if (type_pos == 'D'){
                // On est sur la colone de descente 
                // On peut descendre
                if(pos_index + nb_colonnes +1<= pos_finale_index){
                    sprintf(holder_name_place, "D%d", pos_index+(nb_colonnes + 1));
                    strcpy(path[i], holder_name_place);
                }
            }
            else if (type_pos == 'M'){
                // On est sur la colone de montée
                // On doit monter sauf si on est sur la première place
                if(pos_index ==(nb_colonnes + 1)){
                    // On est sur la première place, on peut aller sur la colonne de descente
                    sprintf(holder_name_place, "D%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "M%d", pos_index - (nb_colonnes + 1));
                    strcpy(path[i], holder_name_place);
                }

            }
            else if (type_pos == 'S'){
                // On va vers la gauche
                if((pos_index-1)%(nb_colonnes + 1)==0){
                    // On est arrivé en bout (début) des étagères
                    sprintf(holder_name_place, "M%d", pos_index-1);
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
                // On doit monter sauf si on est sur la première place
                if(pos_index ==(nb_colonnes + 1)){
                    // On est sur la première place, on peut aller sur la colonne de descente
                    sprintf(holder_name_place, "D%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "M%d", pos_index - (nb_colonnes + 1));
                    strcpy(path[i], holder_name_place);
                }
            }
            if((type_pos=='D') && (type_pos_finale=='B')){
                // Je suis au bon niveau mais je veux aller aux bacs
                // On se déplace
                sprintf(holder_name_place, "B%d" ,pos_index);
                strcpy(path[i], holder_name_place);
            }
            if (type_pos == 'S'){
                // On va vers la gauche
                if((pos_index)%(nb_colonnes + 1)==0){
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
        return response;
    }
    return NULL;
}

int update_shared_memory_stock(Robot *robot, Item_selected selected_item, int index_pos,int nb_colonnes) {
    // Vérifier si index_pos est valide
    if (index_pos < 0 || index_pos >= selected_item.count) {
        fprintf(stderr, "Erreur : index_pos hors limites.\n");
        return -1;
    }

    // Trouver le prochain indice disponible pour un nouvel élément
    int idx = 0;
    while (idx < MAX_WAYPOINTS && robot->item_name[idx][0] != '\0') {
        idx++;
    }

    // Vérifier s'il y a de la place pour ajouter un nouvel élément
    if (idx == MAX_WAYPOINTS) {
        fprintf(stderr, "Erreur : Plus de place disponible dans la structure Robot.\n");
        return -1;
    }

    // Copier le nom de l'article (assurez-vous que la taille de robot->item_name[idx] est suffisante)
    strncpy(robot->item_name[idx], selected_item.item_name, NAME_ITEM_SIZE - 1);
    robot->item_name[idx][NAME_ITEM_SIZE - 1] = '\0';  // Assurez-vous que le nom est bien null-terminé

    // Allouer et copier les positions
    robot->positions[idx] = (nb_colonnes+1)*2*(selected_item.positions[index_pos][0]+1) + selected_item.positions[index_pos][1] + 1;

    // Copier la quantité
    robot->quantities[idx] = selected_item.quantities[index_pos];

    return 0;
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
    if (robot == NULL) {
        return;
    }
    printf("Robot ID: %d\n", robot->ID);
    printf("Current position: %s\n",robot->current_pos);
    for (int i = 0; i < MAX_WAYPOINTS; i++) {
        if (strcmp(robot->item_name[i],"\0"))
            printf("  - Item: %s\n", robot->item_name[i]);
        if (robot->positions[i] != 0)
            printf("  - Position: (%d)\n", robot->positions[i]);
        if (robot->quantities[i] != 0)
            printf("  - Quantity: %d\n", robot->quantities[i]);
        if (strcmp(robot->waypoints[i],"\0"))
            printf("  - Waypoint: %s\n", robot->waypoints[i]);
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

int add_waypoint(Robot *robot, const char *waypoint) {
    int waypoint_count = 0;
    
    // Trouver le nombre actuel de waypoints
    while (waypoint_count < MAX_WAYPOINTS && robot->waypoints[waypoint_count][0] != '\0') {
        waypoint_count++;
    }
    
    if (waypoint_count >= MAX_WAYPOINTS) {
        printf("Max waypoints reached!\n");
        return -1;
    }
    
    strncpy(robot->waypoints[waypoint_count], waypoint, SIZE_POS - 1);
    robot->waypoints[waypoint_count][SIZE_POS - 1] = '\0';
    return 0;
}

int remove_first_waypoint_of_robot(Robot *robot) {
    if (robot->waypoints[0][0] == '\0') {
        fprintf(stderr,"No waypoints to remove!\n");
        return -1;
    }
    
    for (int i = 0; i < MAX_WAYPOINTS - 1; i++) {
        strncpy(robot->waypoints[i], robot->waypoints[i + 1], SIZE_POS);
    }
    
    // Vider le dernier élément
    robot->waypoints[MAX_WAYPOINTS - 1][0] = '\0';

    return 0;
}

void get_current_and_final_pos(Robot* robot,int no,sem_t* sem_robot,char current_pos[SIZE_POS],char pos_finale[SIZE_POS],char type_final_pos,int nb_colonnes,int nb_bac){
    
    CHECK(sem_wait(sem_robot),"sem_wait(sem_memoire_robot)");
    if (type_final_pos == 'B'){
        strcpy(current_pos,robot->current_pos);
        sprintf(pos_finale, "B%d",(nb_colonnes+1)*(2*no+1)%(nb_bac*2*(nb_colonnes+1)));
    }
    else if(type_final_pos == 'P'){
        strcpy(current_pos,robot->current_pos);
        sprintf(pos_finale, "P%d",(2*nb_bac+no+1)*(nb_colonnes+1));
    }
    else if(type_final_pos == 'S'){
        strcpy(current_pos,robot->current_pos);
        sprintf(pos_finale, "S%d",robot->positions[0]);
    }
    CHECK(sem_post(sem_robot),"sem_post(sem_memoire_robot)");

}

int get_index_of_waypoint(char type_pos,int no_pos,int nb_colonnes,int nb_bac){
    // Retourne l'index de la liste de mutex correspondant d'un waypoint
    if(type_pos == 'P'){
        return no_pos/(nb_colonnes+1) -1 -2*nb_bac;
    }
    else if(type_pos == 'B'){
        return (no_pos/(nb_colonnes+1)-1)/2;
    }
    else if(type_pos == 'S'){
        return no_pos/(2*(nb_colonnes+1)) -1;
    }
    else if(type_pos == 'D' || type_pos == 'M'){
        return no_pos/(nb_colonnes+1) -1;
    }
    return -1;
}

void free_mutex(char type_pos,int no_mutex,sem_t* sem_bac[NB_MAX_BAC],sem_t* sem_parking[NB_MAX_ROBOT],sem_t* sem_lignes[NB_MAX_LIGNES],sem_t* sem_colonneNord[2*NB_MAX_LIGNES],sem_t* sem_colonneSud[2*NB_MAX_LIGNES]){
    if(type_pos == 'P'){
        CHECK(sem_post(sem_parking[no_mutex]),"sem_post(sem_parking)");
    }
    else if(type_pos == 'B'){
        CHECK(sem_post(sem_bac[no_mutex]),"sem_post(sem_bac)");
    }
    else if(type_pos == 'S'){
        CHECK(sem_post(sem_lignes[no_mutex]),"sem_post(sem_lignes)");
    }
    else if(type_pos == 'D'){
        CHECK(sem_post(sem_colonneNord[no_mutex]),"sem_post(sem_colonneNord)");
    }
    else if(type_pos == 'M'){
        CHECK(sem_post(sem_colonneSud[no_mutex]),"sem_post(sem_colonneSud)");
    }
}

void generate_waypoints(const char current_pos[SIZE_POS],const char pos_finale[SIZE_POS],Robot* memoire_robot, sem_t* sem_robot,sem_t* sem_bac[NB_MAX_BAC],sem_t* sem_parking[NB_MAX_ROBOT],sem_t* sem_lignes[NB_MAX_LIGNES],sem_t* sem_colonneNord[2*NB_MAX_LIGNES],sem_t* sem_colonneSud[2*NB_MAX_LIGNES],int nb_lignes, int nb_colonnes,int nb_bac){
    // Fonction qui détermine la trajectoire, demande les ressources associées et met à jour une structure
    
    char type_pos;
    int no_mutex;
    int no_next_mutex;

    // Initialisation du chemin
    char path[MAX_WAYPOINTS][SIZE_POS];
    for(int i =0;i<MAX_WAYPOINTS;i++){
        path[i][0] = '\0';
    }

    // On détermine la trajectoire
    trajectoire(current_pos, pos_finale, path,nb_lignes,nb_colonnes);

    // On boucle pour demander les mutex dans l'ordre
    for (int i = 1; i < MAX_WAYPOINTS; i++) { // Commence à 1 car le premier point de la trajectoire (la position courante) ne nous intéresse pas
        if (path[i][0] == '\0') {
            break;
        }

        // Identification de la mutex correspondante
        type_pos = path[i][0];
        no_mutex = get_index_of_waypoint(type_pos,atoi(path[i]+1),nb_colonnes,nb_bac);
        

        // C'est mon tour, je demande la mutex
        if(type_pos == 'P'){
            CHECK(sem_wait(sem_parking[no_mutex]),"sem_wait(sem_parking)");
        }
        else if(type_pos == 'D'){

            if(path[i+1][0]!='B'){

                // On demande la mutex pour avancer sur la colonne
                CHECK(sem_wait(sem_colonneNord[no_mutex]),"sem_wait(sem_colonneNord)");
            }
            else{
                // On doit d'abord demander la mutex pour rentrer dans la zone avec le bac
                no_next_mutex = get_index_of_waypoint(path[i+1][0],atoi(path[i+1]+1),nb_colonnes,nb_bac);
                CHECK(sem_wait(sem_bac[no_next_mutex]),"sem_wait(sem_bac)");
                // Puis on demande la mutex pour le devant de la zone avec le bac
                CHECK(sem_wait(sem_colonneNord[no_mutex]),"sem_wait(sem_colonneNord)");
                // On met à jour les waypoints
                CHECK(sem_wait(sem_robot),"sem_wait(sem_memoire_robot)");
                add_waypoint(memoire_robot, path[i]);
                // On met a jour la position du robot
                strcpy(memoire_robot->current_pos,path[i]);
                CHECK(sem_post(sem_robot),"sem_post(sem_memoire_robot)");
                i++;
            }

        }
        else if(type_pos == 'M'){

            if(path[i+1][0]!='S'){
                // Je ne fais que passer
                // On demande la mutex pour avancer sur la colonne
                CHECK(sem_wait(sem_colonneSud[no_mutex]),"sem_wait(sem_colonneSud)");
            }
            else{
                // On doit d'abord demander la mutex pour rentrer dans l'allée
                no_next_mutex = get_index_of_waypoint(path[i+1][0],atoi(path[i+1]+1),nb_colonnes,nb_bac);
                CHECK(sem_wait(sem_lignes[no_next_mutex]),"sem_wait(sem_lignes)");
                // Puis on demande la mutex pour le devant de l'allée                
                CHECK(sem_wait(sem_colonneSud[no_mutex]),"sem_wait(sem_colonneSud)");
                // On met à jour les waypoints
                CHECK(sem_wait(sem_robot),"sem_wait(sem_memoire_robot)");
                add_waypoint(memoire_robot, path[i]);
                // On met a jour la position du robot
                strcpy(memoire_robot->current_pos,path[i]);
                CHECK(sem_post(sem_robot),"sem_post(sem_memoire_robot)");
                i++;
            }
        }

        // On met à jour au fur et a mesure la liste des WAIPOINTS du robot
        CHECK(sem_wait(sem_robot),"sem_wait(sem_memoire_robot)");
        add_waypoint(memoire_robot, path[i]);
        // On met a jour la position du robot
        strcpy(memoire_robot->current_pos,path[i]);
        CHECK(sem_post(sem_robot),"sem_post(sem_memoire_robot)");
    }
}

void remove_first_item_of_robot(Robot *robot) {
    if (robot->item_name[0][0] == '\0') {
        printf("No item to remove!\n");
        return;
    }
    
    for (int i = 0; i < MAX_WAYPOINTS - 1; i++) {
        strncpy(robot->item_name[i], robot->item_name[i + 1], NAME_ITEM_SIZE);
        robot->positions[i] = robot->positions[i + 1];
        robot->quantities[i] = robot->quantities[i + 1];
    }
    
    // Vider le dernier élément
    robot->item_name[MAX_WAYPOINTS - 1][0] = '\0';
    robot->positions[MAX_WAYPOINTS - 1] = 0;
    robot->quantities[MAX_WAYPOINTS - 1] = 0;
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

/*
* Look into a csv file to find wether the robot with robot_ip is authorized to connect
* Return the robot ID if the robot is authorized, 0 if not found, and -1 if the file cannot be opened
*/
int authorize_robot_connexion(char *file_name, char *robot_ip){
    
    FILE *file = fopen(file_name, "r");
    if (file == NULL){
        return -1;
    }

    char line[MAXOCTETS];
    while (fgets(line, sizeof(line), file)){
        char *ip = strtok(line, ",");
        char *id = strtok(NULL, ",");
        if (ip == NULL || id == NULL){
            continue;
        }
        if (strcmp(ip, robot_ip) == 0){
            fclose(file);
            return atoi(id);
        }
    }
    fclose(file);
    return 0;
}
