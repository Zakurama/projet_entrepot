#include "tcp.h"
#include "utils.h"
#include "ordi_central.h"

#define IP_SIZE 16
#define DEFAULT_LOCALIP "127.0.0.1"
#define LOCAL_IP "127.0.0.1"
#define LOCAL_PORT 5000

int ports[NB_ROBOT] = {8000,8001};

char ip[IP_SIZE];

void bye();
void gestion_robot(int no);
void gestionnaire_inventaire(int client_sd);
void gestionnaire_traj_robot(int no);

int shm[NB_ROBOT];
size_t size_robot = sizeof(Robot);
Robot* robots[NB_ROBOT];

sem_t* sem_memoire_robot[NB_ROBOT];

// Mutex pour l'entrepôt
sem_t* sem_bac[NB_BAC];
sem_t* sem_parking[NB_ROBOT];
sem_t* sem_lignes[NB_LIGNES];
sem_t* sem_colonneNord[2*NB_LIGNES];
sem_t* sem_colonneSud[2*NB_LIGNES];

sem_t* mutex_ecran;

int main(int argc, char *argv[]) {
    
    int nb_processus = 1+2*NB_ROBOT;

    if(argc == 2){
        strncpy(ip, argv[1], IP_SIZE - 1);
        ip[IP_SIZE - 1] = '\0';
    }
    else{
        strncpy(ip, DEFAULT_LOCALIP, IP_SIZE - 1);
        ip[IP_SIZE - 1] = '\0';
    }

    pid_t pid[nb_processus];

    ////// Definition des mutex et des sémaphore nommées

    // ROBOTS
    char memory_robot_name[20];
    char mutex_name[30];
    for(int i = 0;i<NB_ROBOT;i++){
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
    for(int i = 0;i<NB_BAC;i++){
        sprintf(bac_name, "sem_bac[%d]", i);
        CHECK_S(sem_bac[i] = sem_open(bac_name,O_CREAT|O_EXCL,0666,1),"sem_open(sem_bac)");        
    }

    // PARKING
    char parking_name[30];
    for(int i = 0;i<NB_ROBOT;i++){
        sprintf(parking_name, "sem_parking[%d]", i);
        CHECK_S(sem_parking[i] = sem_open(parking_name,O_CREAT|O_EXCL,0666,1),"sem_open(parking_name)");        
    }

    // LIGNES
    char line_name[30];
    for(int i = 0;i<NB_LIGNES;i++){
        sprintf(line_name, "sem_lignes[%d]", i);
        CHECK_S(sem_lignes[i] = sem_open(line_name,O_CREAT|O_EXCL,0666,1),"sem_open(line_name)");        
    }

    // COLONNES
    char colonne_nord_name[30];
    char colonne_sud_name[30];
    for(int i = 0;i<2*NB_LIGNES;i++){
        sprintf(colonne_nord_name, "sem_colonneNord[%d]", i);
        CHECK_S(sem_colonneNord[i] = sem_open(colonne_nord_name,O_CREAT|O_EXCL,0666,1),"sem_open(colonne_nord_name)"); 
        sprintf(colonne_sud_name, "sem_colonneSud[%d]", i);
        CHECK_S(sem_colonneSud[i] = sem_open(colonne_sud_name,O_CREAT|O_EXCL,0666,1),"sem_open(colonne_sud_name)");        
    }

    // ECRAN
    CHECK_S(mutex_ecran = sem_open("mutex_ecran",O_CREAT|O_EXCL,0666,1),"sem_open(mutex_ecran)"); 

    ////// FIN definition des mutex et des sémaphore nommées

    // Permet de faire le cleanning des sémaphores lors des exits
    atexit(bye);// bye detruit les semaphores

    // Avec ça le père sera "imunisé" au Ctrl-C mais pas ses fils (on va réactiver le SIGINT pour eux) ! 
    // Du coup ils vont tous se terminer, sauf le père qui va pouvoir les récupérer et terminer correctement est donc faire le nettoyage des sémaphores
    // Masque SIGINT pour le père
    sigset_t Mask,OldMask;
    CHECK(sigemptyset(&Mask), "sigemptyset()");
    CHECK(sigaddset(&Mask , SIGINT), "sigaddset(SIGINT)");
    CHECK(sigprocmask(SIG_SETMASK , &Mask , &OldMask), "sigprocmask()");

    // Création des fils du processus père (un par train)
    for (int i=0;i<nb_processus;i++){
        CHECK(pid[i]=fork(),"fork(pid)");
        if (pid[i]==0){
            // Démasque SIGINT
            CHECK(sigprocmask(SIG_SETMASK , &OldMask , NULL), "sigprocmask()");
            if(i==0){
                // Gestionnaire communication inventaire
                int se_inventaire = 0; // la définir au préalable
                init_tcp_socket(&se_inventaire,LOCAL_IP,LOCAL_PORT,1);
                listen_to(se_inventaire);
                int client_sd = accept_client(se_inventaire);
                gestionnaire_inventaire(client_sd);
            }
            if(i>0 && i<=NB_ROBOT){
                // Processus de gestion des robots
                gestion_robot(i-1);
            }
            else if(i>NB_ROBOT && i< nb_processus){
                // Processus de gestion des trajecoires
                gestionnaire_traj_robot(i-NB_ROBOT-1);
            }

        }
    }

    // Processus Père
    // Attente de la terminaison des fils
    for (int i=0;i<nb_processus;i++){
        int status;
        CHECK(wait(&status), "wait()");
    }

    return 0;
}

void bye(){

    // Suppression des mutex et des mémoires partagées

    // ROBOT
    char nom_memoire[20];
    char mutex_name[30];
    for(int i = 0;i<NB_ROBOT;i++){
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
    for(int i = 0;i<NB_BAC;i++){
        sprintf(bac_name, "sem_bac[%d]", i);
        CHECK(sem_close(sem_bac[i]),"sem_close(bac_name)");
        CHECK(sem_unlink(bac_name),"sem_unlink(bac_name)");
    }

    // PARKING
    char parking_name[30];
    for(int i = 0;i<NB_ROBOT;i++){
        sprintf(parking_name, "sem_parking[%d]", i);
        CHECK(sem_close(sem_parking[i]),"sem_close(sem_parking)");
        CHECK(sem_unlink(parking_name),"sem_unlink(parking_name)");
    }

    // LIGNES
    char line_name[30];
    for(int i = 0;i<NB_LIGNES;i++){
        sprintf(line_name, "sem_lignes[%d]", i);
        CHECK(sem_close(sem_lignes[i]),"sem_close(sem_lignes)");
        CHECK(sem_unlink(line_name),"sem_unlink(line_name)");
    }

    // COLONNES
    char colonne_nord_name[30];
    char colonne_sud_name[30];
    for(int i = 0;i<2*NB_LIGNES;i++){
        sprintf(colonne_nord_name, "sem_colonneNord[%d]", i);
        sprintf(colonne_sud_name, "sem_colonneSud[%d]", i);
        CHECK(sem_close(sem_colonneNord[i]),"sem_close(sem_colonneNord)");
        CHECK(sem_unlink(colonne_nord_name),"sem_unlink(colonne_nord_name)");
        CHECK(sem_close(sem_colonneSud[i]),"sem_close(sem_colonneSud)");
        CHECK(sem_unlink(colonne_sud_name),"sem_unlink(colonne_sud_name)");
    }

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
        recev_message(client_sd, buffer_reception_pos_articles); // la liste des positions

        char *error =  convert_request_strings_to_lists(buffer_reception_ID_articles, buffer_reception_pos_articles, item_names_requested, L_n_requested, L_n_stock, L_x_stock, L_y_stock, item_names_stock, &count_requested, count_stock,&nb_items);
        if (error != NULL) {
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
                update_shared_memory_stock(robots[ID_robot],selected_items[i],j);
                CHECK(sem_post(sem_memoire_robot[ID_robot]),"sem_post(sem_memoire_robot)");
                ID_robot = (ID_robot + 1) % NB_ROBOT; // Pour l'instant pas de choix optimal du robot, on prend juste à son tour les robots
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
        get_current_and_final_pos(robots[no],no,sem_memoire_robot[no],current_pos,pos_finale,'S');

        // On vérifie si on ne se trouve pas déjà au bon endroit
        if(strcmp(current_pos,pos_finale)!=0){
            generate_waypoints(current_pos,pos_finale,robots[no],sem_memoire_robot[no]);
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
        get_current_and_final_pos(robots[no],no,sem_memoire_robot[no],current_pos,pos_finale,'B');
        generate_waypoints(current_pos,pos_finale,robots[no],sem_memoire_robot[no]);

        // On vide dans le bac
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        robots[no]->hold_items = 0;
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");

        // On retourne au parking
        get_current_and_final_pos(robots[no],no,sem_memoire_robot[no],current_pos,pos_finale,'P');
        generate_waypoints(current_pos,pos_finale,robots[no],sem_memoire_robot[no]);
    }
}

void gestion_robot(int no){

    // Determinaison de la postion de parking
    char parking_spot[SIZE_POS];
    sprintf(parking_spot, "P%d", (NB_COLONNES+1)*5 + no*(NB_COLONNES+1));

    // Initialisation de la mémoire partagée
    CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
    robots[no]->ID = no;
    strcpy(robots[no]->current_pos,parking_spot);
    robots[no]->hold_items=0;
    CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
    
    while(1){

        // Holder des waypoints
        char waypoints[MAX_WAYPOINTS][SIZE_POS];

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
        for(int i =0;i<MAX_WAYPOINTS;i++){
            if(!strcmp(waypoints[i],"\0")){
                break;
            }
            printf(" %s ",waypoints[i]);
        }
        printf("\n");
        printf("FIN DE L'AFFICHAGE DES WAYPOINTS du robot %d\n",no);
        CHECK(sem_post(mutex_ecran),"sem_post(mutex_ecran)");

        // On envoie les waypoints au robot
        // TODO

        // On attend le retour du robot
        // TODO

        // On libère les mutex
        // TODO
    }

}
