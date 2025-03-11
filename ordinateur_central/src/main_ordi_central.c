#include "tcp.h"
#include "utils.h"
#include "ordi_central.h"

#define IP_SIZE 16
#define DEFAULT_LOCALIP "127.0.0.1"
#define LOCAL_PORT 5000
#define PORT_ROBOT 8000

char ip[IP_SIZE];

void bye();

void gestionnaire_inventaire(int client_sd);
void gestion_flotte(int *nb_robots, char *ip);

void gestion_communication_robot(int no, int se, int *nb_robots);
void gestionnaire_traj_robot(int no);

// MEMOIRES PARTAGEES
int *nb_colonnes;
int *nb_lignes;
int *nb_robots;
Robot* robots[NB_MAX_ROBOT];
Liste_pos_waypoints * liste_waypoints; 

int shm_colonnes, shm_lignes;
int shm_nb_robots;
int shm[NB_MAX_ROBOT];
int shm_liste_waypoints;

size_t size_int = sizeof(int);
size_t size_robot = sizeof(Robot);
size_t size_liste_waypoints = sizeof(Liste_pos_waypoints);

// SEMAPHORE ACCES MEMOIRE ROBOT
sem_t* sem_memoire_robot[NB_MAX_ROBOT];

// SEMAPHORES ENTREPOT
sem_t* sem_bac[NB_MAX_BAC];
sem_t* sem_parking[NB_MAX_ROBOT];
sem_t* sem_lignes[NB_MAX_LIGNES];
sem_t* sem_colonneNord[2*NB_MAX_LIGNES];
sem_t* sem_colonneSud[2*NB_MAX_LIGNES];

// SEMAPHORE ACCES ECRAN
sem_t* mutex_ecran;

int main(int argc, char *argv[]) {


    if(argc == 2){
        strncpy(ip, argv[1], IP_SIZE);
        ip[IP_SIZE] = '\0';
    }
    else{
        strncpy(ip, DEFAULT_LOCALIP, IP_SIZE);
        ip[IP_SIZE] = '\0';
    }

    // Mémoire partagée pour le nombre de robot
    CHECK(shm_nb_robots = shm_open("nb_robots", O_CREAT | O_RDWR, 0666), "shm_open(nb_robots)");
    CHECK(ftruncate(shm_nb_robots, size_int), "ftruncate(shm_nb_robots)");
    CHECK_MAP(nb_robots = mmap(0, size_int, PROT_READ | PROT_WRITE, MAP_SHARED, shm_nb_robots, 0), "mmap(nb_robots)");

    // Create shared memory for nb_colonnes and nb_lignes
    CHECK(shm_colonnes = shm_open("nb_colonnes", O_CREAT | O_RDWR, 0666), "shm_open(nb_colonnes)");
    CHECK(ftruncate(shm_colonnes, size_int), "ftruncate(shm_colonnes)");
    CHECK_MAP(nb_colonnes = mmap(0, size_int, PROT_READ | PROT_WRITE, MAP_SHARED, shm_colonnes, 0), "mmap(nb_colonnes)");

    CHECK(shm_lignes = shm_open("nb_lignes", O_CREAT | O_RDWR, 0666), "shm_open(nb_lignes)");
    CHECK(ftruncate(shm_lignes, size_int), "ftruncate(shm_lignes)");
    CHECK_MAP(nb_lignes = mmap(0, size_int, PROT_READ | PROT_WRITE, MAP_SHARED, shm_lignes, 0), "mmap(nb_lignes)");

    // Initialize shared memory values
    *nb_colonnes = DEFAULT_NB_COLONNES;
    *nb_lignes = DEFAULT_NB_LIGNES;
    *nb_robots = 0;

    ////// Definition des mutex et des sémaphore nommées

    // ROBOTS
    char memory_robot_name[20];
    char mutex_name[30];
    for(int i = 0;i<NB_MAX_ROBOT;i++){
        // Gestion des mémoires partagées
        sprintf(memory_robot_name, "robots_data[%d]", i);
        CHECK(shm[i] = shm_open(memory_robot_name, O_CREAT | O_RDWR, 0666),"shm_open(robots_data)");
        CHECK(ftruncate(shm[i], size_robot),"ftruncate(shm)");
        CHECK_MAP(robots[i] = mmap(0, size_robot, PROT_READ | PROT_WRITE, MAP_SHARED, shm[i], 0),"mmap");

        // Gestion des mutex pour l'acces memoire
        sprintf(mutex_name, "sem_memoire_robot[%d]", i);
        CHECK_S(sem_memoire_robot[i] = sem_open(mutex_name,O_CREAT|O_EXCL,0666,1),"sem_open(sem_memoire_robot)");
        
    }

    // BACS
    char bac_name[30];
    for(int i = 0;i<NB_MAX_BAC;i++){
        sprintf(bac_name, "sem_bac[%d]", i);
        CHECK_S(sem_bac[i] = sem_open(bac_name,O_CREAT|O_EXCL,0666,1),"sem_open(sem_bac)");        
    }

    // PARKING
    char parking_name[30];
    for(int i = 0;i<NB_MAX_ROBOT;i++){
        sprintf(parking_name, "sem_parking[%d]", i);
        CHECK_S(sem_parking[i] = sem_open(parking_name,O_CREAT|O_EXCL,0666,1),"sem_open(parking_name)");        
    }

    // LIGNES
    char line_name[30];
    for(int i = 0;i<NB_MAX_LIGNES;i++){
        sprintf(line_name, "sem_lignes[%d]", i);
        CHECK_S(sem_lignes[i] = sem_open(line_name,O_CREAT|O_EXCL,0666,1),"sem_open(line_name)");        
    }

    // COLONNES
    char colonne_nord_name[30];
    char colonne_sud_name[30];
    for(int i = 0;i<2*NB_MAX_LIGNES;i++){
        sprintf(colonne_nord_name, "sem_colonneNord[%d]", i);
        CHECK_S(sem_colonneNord[i] = sem_open(colonne_nord_name,O_CREAT|O_EXCL,0666,1),"sem_open(colonne_nord_name)"); 
        sprintf(colonne_sud_name, "sem_colonneSud[%d]", i);
        CHECK_S(sem_colonneSud[i] = sem_open(colonne_sud_name,O_CREAT|O_EXCL,0666,1),"sem_open(colonne_sud_name)");        
    }

    // ECRAN
    CHECK_S(mutex_ecran = sem_open("mutex_ecran",O_CREAT|O_EXCL,0666,1),"sem_open(mutex_ecran)"); 

    ////// FIN definition des mutex et des sémaphore nommées

    // Création de la mémoire partagée pour la liste de waypoints

    CHECK(shm_liste_waypoints = shm_open("liste_waypoints", O_CREAT | O_RDWR, 0666),"shm_open(shm_liste_waypoints)");
    CHECK(ftruncate(shm_liste_waypoints, size_liste_waypoints),"ftruncate(shm_liste_waypoints)");
    CHECK_MAP(liste_waypoints = mmap(0, size_liste_waypoints, PROT_READ | PROT_WRITE, MAP_SHARED, shm_liste_waypoints, 0),"mmap");

    waypoints_creation(liste_waypoints, DEFAULT_HEDGE_3, DEFAULT_HEDGE_4, DEFAULT_HEDGE_5, *nb_colonnes, *nb_lignes, NB_MAX_ROBOT);

    // Permet de faire le cleanning des sémaphores lors des exits
    atexit(bye);// bye detruit les semaphores

    // Avec ça le père sera "imunisé" au Ctrl-C mais pas ses fils (on va réactiver le SIGINT pour eux) ! 
    // Du coup ils vont tous se terminer, sauf le père qui va pouvoir les récupérer et terminer correctement est donc faire le nettoyage des sémaphores
    // Masque SIGINT pour le père
    sigset_t Mask,OldMask;
    CHECK(sigemptyset(&Mask), "sigemptyset()");
    CHECK(sigaddset(&Mask , SIGINT), "sigaddset(SIGINT)");
    CHECK(sigprocmask(SIG_SETMASK , &Mask , &OldMask), "sigprocmask()");

    pid_t pid[2];
    // Création des fils du processus père
    for (int i=0;i<2;i++){
        CHECK(pid[i]=fork(),"fork(pid)");
        if (pid[i]==0){
            // Démasque SIGINT
            CHECK(sigprocmask(SIG_SETMASK , &OldMask , NULL), "sigprocmask()");
            if(i==0){
                // Gestionnaire communication inventaire
                int se_inventaire = 0;
                init_tcp_socket(&se_inventaire, ip, LOCAL_PORT, 1);
                listen_to(se_inventaire);
                int client_sd = accept_client(se_inventaire);
                gestionnaire_inventaire(client_sd);
            }
            if(i==1){
                // Processus de gestion de la flotte de robots
                gestion_flotte(nb_robots, ip);
            }

        }
    }

    // Processus Père
    // Attente de la terminaison des fils
    for (int i=0;i<2;i++){
        int status;
        CHECK(wait(&status), "wait()");
    }

    return EXIT_SUCCESS;
}

void bye(){

    // Suppression des mutex et des mémoires partagées

    // ROBOT
    char nom_memoire[20];
    char mutex_name[30];
    for(int i = 0;i<NB_MAX_ROBOT;i++){
        // Suppression de la mémoire partagée
        sprintf(nom_memoire, "robots_data[%d]", i);
        CHECK(munmap(robots[i], size_robot),"munmap(robots_data)");
        CHECK(close(shm[i]),"close(shm)");
        CHECK(shm_unlink(nom_memoire),"shm_unlink(robots_data)");

        // Fermeture et supression des sémaphores nommées
        sprintf(mutex_name, "sem_memoire_robot[%d]", i);
        CHECK(sem_close(sem_memoire_robot[i]),"sem_close(sem_memoire_robot)");
        CHECK(sem_unlink(mutex_name),"sem_unlink(sem_memoire_robot)");
    }

    // ECRAN
    CHECK(sem_close(mutex_ecran),"sem_close(mutex_ecran)");
    CHECK(sem_unlink("mutex_ecran"),"sem_unlink(mutex_ecran)");

    // BAC
    char bac_name[30];
    for(int i = 0;i<NB_MAX_BAC;i++){
        sprintf(bac_name, "sem_bac[%d]", i);
        CHECK(sem_close(sem_bac[i]),"sem_close(bac_name)");
        CHECK(sem_unlink(bac_name),"sem_unlink(bac_name)");
    }

    // PARKING
    char parking_name[30];
    for(int i = 0;i<NB_MAX_ROBOT;i++){
        sprintf(parking_name, "sem_parking[%d]", i);
        CHECK(sem_close(sem_parking[i]),"sem_close(sem_parking)");
        CHECK(sem_unlink(parking_name),"sem_unlink(parking_name)");
    }

    // LIGNES
    char line_name[30];
    for(int i = 0;i<NB_MAX_LIGNES;i++){
        sprintf(line_name, "sem_lignes[%d]", i);
        CHECK(sem_close(sem_lignes[i]),"sem_close(sem_lignes)");
        CHECK(sem_unlink(line_name),"sem_unlink(line_name)");
    }

    // COLONNES
    char colonne_nord_name[30];
    char colonne_sud_name[30];
    for(int i = 0;i<2*NB_MAX_LIGNES;i++){
        sprintf(colonne_nord_name, "sem_colonneNord[%d]", i);
        sprintf(colonne_sud_name, "sem_colonneSud[%d]", i);
        CHECK(sem_close(sem_colonneNord[i]),"sem_close(sem_colonneNord)");
        CHECK(sem_unlink(colonne_nord_name),"sem_unlink(colonne_nord_name)");
        CHECK(sem_close(sem_colonneSud[i]),"sem_close(sem_colonneSud)");
        CHECK(sem_unlink(colonne_sud_name),"sem_unlink(colonne_sud_name)");
    }

    // LISTE WAYPOINTS
    CHECK(munmap(liste_waypoints, size_liste_waypoints),"munmap(liste_waypoints)");
    CHECK(close(shm_liste_waypoints),"close(shm_liste_waypoints)");
    CHECK(shm_unlink("liste_waypoints"),"shm_unlink(liste_waypoints)");

    // NB_LIGNES NB_COLONNES NB_ROBOTS

    CHECK(munmap(nb_lignes, size_int),"munmap(nb_lignes)");
    CHECK(close(shm_lignes),"close(shm_lignes)");
    CHECK(shm_unlink("nb_lignes"),"shm_unlink(nb_lignes)");

    CHECK(munmap(nb_colonnes, size_int),"munmap(nb_colonnes)");
    CHECK(close(shm_colonnes),"close(shm_colonnes)");
    CHECK(shm_unlink("nb_colonnes"),"shm_unlink(nb_colonnes)");

    CHECK(munmap(nb_robots, size_int),"munmap(nb_robots)");
    CHECK(close(shm_nb_robots),"close(shm_nb_robots)");
    CHECK(shm_unlink("nb_robots"),"shm_unlink(nb_robots)");

}

void gestionnaire_inventaire(int client_sd){
    // Demande du client
    char buffer_reception_ID_articles[MAXOCTETS];
    char buffer_reception_pos_articles[MAXOCTETS];
    char buffer_emission[MAXOCTETS];
    char *item_names_requested[MAX_ARTICLES_LISTE_ATTENTE];
    int L_n_requested[MAX_ARTICLES_LISTE_ATTENTE];

    // Stocks
    int *L_n_stock[MAX_ARTICLES_LISTE_ATTENTE];
    int *L_x_stock[MAX_ARTICLES_LISTE_ATTENTE];
    int *L_y_stock[MAX_ARTICLES_LISTE_ATTENTE];
    char *item_names_stock[MAX_ARTICLES_LISTE_ATTENTE];
    int count_stock[MAX_ARTICLES_LISTE_ATTENTE]; // nombre de positions par article
    int count_requested; // nombre d'articles demandés
    int nb_items;

    // Articles choisis
    char *chosen_item_names[MAX_ARTICLES_LISTE_ATTENTE];
    int *chosen_x_positions[MAX_ARTICLES_LISTE_ATTENTE];
    int *chosen_y_positions[MAX_ARTICLES_LISTE_ATTENTE];
    int *chosen_quantities[MAX_ARTICLES_LISTE_ATTENTE];
    int chosen_counts[MAX_ARTICLES_LISTE_ATTENTE];
    Item_selected selected_items[MAX_ARTICLES_LISTE_ATTENTE];

    int ID_robot = 0;

    while (1){
        // On récupère les demandes de l'inventaire
        recev_message(client_sd, buffer_reception_ID_articles); // la liste des articles (ID)
    
        // à ce moment soit l'inventaire a transmit une commande de clients 
        // soit il a transmit le nombre de lignes ou de colonnes de l'inventaire
        
        // On vérifie si l'inventaire a envoyé une commande de modification de la taille de l'inventaire
        int new_size = 0;
        char size_type[MAXOCTETS];
        if (sscanf(buffer_reception_ID_articles, "%[^,],%d", size_type, &new_size) == 2) {
            if (strcmp(size_type, "rows") == 0) {
                // Modification du nombre de lignes
                *nb_lignes = new_size;
                waypoints_creation (liste_waypoints, DEFAULT_HEDGE_3, DEFAULT_HEDGE_4, DEFAULT_HEDGE_5, *nb_colonnes, *nb_lignes, NB_MAX_ROBOT);
            } else if (strcmp(size_type, "columns") == 0) {
                // Modification du nombre de colonnes
                *nb_colonnes = new_size;
                waypoints_creation (liste_waypoints, DEFAULT_HEDGE_3, DEFAULT_HEDGE_4, DEFAULT_HEDGE_5, *nb_colonnes, *nb_lignes, NB_MAX_ROBOT);
            }
            else {
                strcpy(buffer_emission, "Invalid size type");
                send_message(client_sd, buffer_emission);
                continue;
            }
            strcpy(buffer_emission, "Size updated successfully");
            send_message(client_sd, buffer_emission);
            continue;
        }
        // L'inventaire à envoyé une requête de commande
        // On récupère les positions des articles en stock
        recev_message(client_sd, buffer_reception_pos_articles); // la liste des positions

        char *error =  convert_request_strings_to_lists(buffer_reception_ID_articles, buffer_reception_pos_articles, item_names_requested, L_n_requested, L_n_stock, L_x_stock, L_y_stock, item_names_stock, &count_requested, count_stock,&nb_items);
        if (error != NULL) {
            return;
        }

        if(*nb_robots==0){
            // Il n'y a pas de robot dans l'entrepôt
            printf("Il n'y a pas de robot connectés\n");
            return;
        }

        // On fait le choix des articles dans les stocks
        int nb_selected = choose_items_stocks(item_names_requested, L_n_requested, count_requested,item_names_stock, L_n_stock, L_x_stock, L_y_stock, count_stock,selected_items);

        // On informe l'inventaire qu'on a bien pris en compte sa demande (on indique quels articles de l'inventaire vont être pris)
        convert_items_to_lists(selected_items, nb_selected,chosen_item_names,chosen_x_positions,chosen_y_positions,chosen_quantities,chosen_counts);
        strcpy(buffer_emission,create_inventory_string(nb_items, MAX_ARTICLES_LISTE_ATTENTE, chosen_counts, chosen_quantities, chosen_x_positions, chosen_y_positions, chosen_item_names));
        send_message(client_sd,buffer_emission);

        // On choisit le robot qui traitera la tâche et la position de l'article souhaité en stock
        for (int i = 0; i < nb_selected; i++) {
            for (int j = 0;j<selected_items[i].count;j++){
                // On met à jour sa mémoire partagée
                CHECK(sem_wait(sem_memoire_robot[ID_robot]),"sem_wait(sem_memoire_robot)");
                update_shared_memory_stock(robots[ID_robot],selected_items[i],j,*nb_colonnes);
                CHECK(sem_post(sem_memoire_robot[ID_robot]),"sem_post(sem_memoire_robot)");
                ID_robot = (ID_robot + 1) % (*nb_robots); // Pour l'instant pas de choix optimal du robot, on prend juste à son tour les robots
            }
        }

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

void gestion_flotte(int *nb_robots, char *ip){
    int se;
    init_tcp_socket(&se, ip, PORT_ROBOT, 1);
    listen(se, MAXCLIENTS);

    while (1){
        struct sockaddr_in adrclient;
        socklen_t adrclient_len = sizeof(adrclient);
        int client_sd = accept(se, (struct sockaddr *)&adrclient, &adrclient_len);
        CHECK_ERROR(client_sd, -1, "Erreur de accept !!!\n");

        int robot_id = authorize_robot_connexion("robots.csv", inet_ntoa(adrclient.sin_addr));
        CHECK_ERROR(robot_id, -1, "Erreur d'autorisation de connexion !!!\n");
        if (robot_id == 0) {
            close_socket(&client_sd);
            continue;
        }
        if (*nb_robots >= NB_MAX_ROBOT) {
            close_socket(&client_sd);
            continue;
        }
        *nb_robots += 1;

        // Le robot a le droit de se connecter, on crée donc ses gestionnaires
        pid_t pid[2];
        for (int i=0;i<2;i++){
            CHECK(pid[i]=fork(),"fork(pid)");
            if (pid[i]==0){
                if(i==0){
                    // Gestionnaire de communication avec le robot
                    gestion_communication_robot(robot_id-1, client_sd, nb_robots);
                }
                if(i==1){
                    // Processus de gestion de trajectoire du robot
                    gestionnaire_traj_robot(robot_id-1);
                }
            }
        }
    }
}

void gestionnaire_traj_robot(int no){
    while(1){

        char current_pos[SIZE_POS];
        char pos_finale[SIZE_POS];    

        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        int is_item_in_memory = strcmp(robots[no]->item_name[0],"\0");
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
        
        if(!is_item_in_memory){
            // Il n'y a pas d'article à traiter pour l'instant. 
            // On attend un peu
            sleep(2);
            continue;
        }
        // Le robot doit aller chercher l'article
        get_current_and_final_pos(robots[no],no,sem_memoire_robot[no],current_pos,pos_finale,'S',*nb_colonnes,DEFAULT_NB_BACS);
        printf("Position initiale :%s, position finale %s\n",current_pos,pos_finale);
        // On vérifie si on ne se trouve pas déjà au bon endroit
        if(strcmp(current_pos,pos_finale)!=0){
            generate_waypoints(current_pos,pos_finale,robots[no],sem_memoire_robot[no],sem_bac,sem_parking,sem_lignes,sem_colonneNord,sem_colonneSud,*nb_lignes,*nb_colonnes,DEFAULT_NB_BACS);
        }

        // On regarde combien d'articles on peut porter
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        if(robots[no]->quantities[0] + robots[no]->hold_items > MAX_ARTICLES_PORTES){
            // On ne peut pas porter tout d'un coup
            // On met juste a jour la liste des quantités
            robots[no]->quantities[0] = robots[no]->quantities[0] - (MAX_ARTICLES_PORTES - robots[no]->hold_items);
            robots[no]->hold_items = MAX_ARTICLES_PORTES;
        }
        else{
            // On peut porter tout d'un coup
            robots[no]->hold_items = robots[no]->hold_items + robots[no]->hold_items;
            // On peut supprimer l'article de la liste
            remove_first_item_of_robot(robots[no]);
        }
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");


        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        is_item_in_memory = strcmp(robots[no]->item_name[0],"\0");
        int nb_item_hold = robots[no]->hold_items;
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
        
        if(!(!is_item_in_memory || nb_item_hold == MAX_ARTICLES_PORTES)){
            // il y a encore des articles a tratier avant d'aller au bac
            continue;
        }

        // On doit aller au bac
        get_current_and_final_pos(robots[no],no,sem_memoire_robot[no],current_pos,pos_finale,'B',*nb_colonnes,DEFAULT_NB_BACS);
        generate_waypoints(current_pos,pos_finale,robots[no],sem_memoire_robot[no],sem_bac,sem_parking,sem_lignes,sem_colonneNord,sem_colonneSud,*nb_lignes,*nb_colonnes,DEFAULT_NB_BACS);

        // On vide dans le bac
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        robots[no]->hold_items = 0;
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");

        // On retourne au parking
        get_current_and_final_pos(robots[no],no,sem_memoire_robot[no],current_pos,pos_finale,'P',*nb_colonnes,DEFAULT_NB_BACS);
        generate_waypoints(current_pos,pos_finale,robots[no],sem_memoire_robot[no],sem_bac,sem_parking,sem_lignes,sem_colonneNord,sem_colonneSud,*nb_lignes,*nb_colonnes,DEFAULT_NB_BACS);
    }
}

void gestion_communication_robot(int no, int se, int *nb_robots){

    // Determinaison de la postion de parking
    char parking_spot[SIZE_POS];
    sprintf(parking_spot, "P%d", (*nb_colonnes+1)*(2*DEFAULT_NB_BACS+1) + no*(*nb_colonnes+1));

    // On prends la mutex de cette place
    int no_mutex = get_index_of_waypoint('P',(*nb_colonnes+1)*(2*DEFAULT_NB_BACS+1) + no*(*nb_colonnes+1),*nb_colonnes,DEFAULT_NB_BACS);
    CHECK(sem_wait(sem_parking[no_mutex]),"sem_wait(sem_parking)");

    // Initialisation de la mémoire partagée
    CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
    robots[no]->ID = no;
    strcpy(robots[no]->current_pos,parking_spot);
    robots[no]->hold_items=0;
    CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
    
    // Position initiale pour chaque trajet
    char pos_init[SIZE_POS];
    strcpy(pos_init,parking_spot);

    while(1){

        // Holder des waypoints
        char waypoints[MAX_WAYPOINTS][SIZE_POS];
        for(int i = 0;i<MAX_WAYPOINTS;i++){
            waypoints[i][0]='\0';
        }

        // On regarde si il y a de nouveau waypoints dans la mémoire partagée
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        int is_waypoints_in_memory = strcmp(robots[no]->waypoints[0],"\0");
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
        
        if(!is_waypoints_in_memory){
            // Il n'y a pas de waypoints à traiter pour l'instant. On attend un peu
            sleep(2);
            continue;
        }

        // On récupère les waypoints à traiter
        int i = 0;
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        while (strcmp(robots[no]->waypoints[0],"\0")){
            strcpy(waypoints[i],robots[no]->waypoints[0]);
            // On supprime le waypoint de la mémoire partagée
            remove_first_waypoint_of_robot(robots[no]);
            i++;
        }
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");

        //TESTS : On affiche les waypoints extrait de la mémoire partagée
        CHECK(sem_wait(mutex_ecran),"sem_wait(mutex_ecran)");
        printf("AFFICHAGE DES WAYPOINTS du robot %d\n",no);
        Point point_coord;
        for(int i =0;i<MAX_WAYPOINTS;i++){
            if(!strcmp(waypoints[i],"\0")){
                break;
            }
            find_waypoint(liste_waypoints, waypoints[i], &point_coord);
            printf(" %s (%f,%f) ",waypoints[i],point_coord.x,point_coord.y);
        }
        printf("\n");
        printf("FIN DE L'AFFICHAGE DES WAYPOINTS du robot %d\n",no);
        CHECK(sem_post(mutex_ecran),"sem_post(mutex_ecran)");

        // On envoie les waypoints au robot
        // TODO

        // On attend le retour du robot
        // TODO

        // Libèration des mutex 
        // On libère d'abord la mutex de la position initiale (qui n'apparait pas dans la liste des waypoints)
        char type_pos = pos_init[0];
        no_mutex = get_index_of_waypoint(type_pos,atoi(pos_init+1),*nb_colonnes,DEFAULT_NB_BACS);
        free_mutex(type_pos,no_mutex,sem_bac,sem_parking,sem_lignes,sem_colonneNord,sem_colonneSud);

        // On libère la suite (on libère tout sauf la position finale)
        int index_pos_finale;
        for(int i =0;i<MAX_WAYPOINTS;i++){
            if(!strcmp(waypoints[i+1],"\0")){
                index_pos_finale = i;
                break;
            }
            type_pos = waypoints[i][0];
            no_mutex = get_index_of_waypoint(type_pos,atoi(waypoints[i]+1),*nb_colonnes,DEFAULT_NB_COLONNES);
            free_mutex(type_pos,no_mutex,sem_bac,sem_parking,sem_lignes,sem_colonneNord,sem_colonneSud);
        }

        // On définit la prochaine position initiale
        strcpy(pos_init,waypoints[index_pos_finale]);

    }
}

