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

    char nom_memoire[20];
    char mutex_name[30];
    for(int i = 0;i<NB_ROBOT;i++){
        // Gestion des mémoires partagées
        sprintf(nom_memoire, "robots_data[%d]", i);
        CHECK(shm[i] = shm_open(nom_memoire, O_CREAT | O_RDWR, 0666),"shm_open(robots_data)");
        CHECK(ftruncate(shm[i], size_robot),"ftruncate(shm)");
        CHECK_MAP(robots[i] = mmap(0, size_robot, PROT_READ | PROT_WRITE, MAP_SHARED, shm[i], 0),"mmap");

        // Gestion des mutex pour l'acces memoire
        sprintf(mutex_name, "sem_memoire_robot[%d]", i);
        CHECK_S(sem_memoire_robot[i] = sem_open(mutex_name,O_CREAT|O_EXCL,0666,1),"sem_open(sem_memoire_robot)");        

    }

    // gestion de la mutex pour l'accès a l'écran
    CHECK_S(mutex_ecran = sem_open("mutex_ecran",O_CREAT|O_EXCL,0666,1),"sem_open(mutex_ecran)"); 

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

    CHECK(sem_close(mutex_ecran),"sem_close(mutex_ecran)");
    CHECK(sem_unlink("mutex_ecran"),"sem_unlink(mutex_ecran)");
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

        char path1[MAX_WAYPOINTS][SIZE_POS];

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
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        strcpy(current_pos,robots[no]->current_pos);
        sprintf(pos_finale, "S%d%d",robots[no]->positions[0][0],robots[no]->positions[0][1]);
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
        
        // On vérifie si on ne se trouve pas déjà au bon endroit
        if(strcmp(current_pos,pos_finale)!=0){
            // On détermine la trajectoire
            trajectoire(current_pos, pos_finale, path1);

            // On boucle pour demander les mutex dans l'ordre
            for (int i = 0; i < MAX_WAYPOINTS; i++) {
                if (path1[i][0] == '\0') {
                    break;
                }
                // On se met sur la file d'attente
                // Tant que ce n'est pas mon tour j'attend
                // TODO

                // C'est mon tour, je demande la mutex
                // TODO
                // On met à jour au fur et a mesure la liste des WAIPOINTS du robot
                CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
                add_waypoint(robots[no], path1[i]);
                // On met a jour la position du robot
                strcpy(robots[no]->current_pos,path1[i]);
                CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
            }
        
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
        char path2[MAX_WAYPOINTS][SIZE_POS];
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        strcpy(current_pos,robots[no]->current_pos);
        sprintf(pos_finale, "B%d",(NB_COLONNES+1)*(no+1));
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");

        // On détermine la trajectoire
        trajectoire(current_pos, pos_finale, path2);

        // On boucle pour demander les mutex dans l'ordre
        for (int i = 0; i < MAX_WAYPOINTS; i++) {
            if (path2[i][0] == '\0') {
                break;
            }
            // On se met sur la file d'attente
            // Tant que ce n'est pas mon tour j'attend
            // TODO
 
            // C'est mon tour, je demande la mutex
            // TODO
            // On met à jour au fur et a mesure la liste des WAIPOINTS du robot
            CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
            add_waypoint(robots[no], path2[i]);
            // On met a jour la position du robot
            strcpy(robots[no]->current_pos,path2[i]);
            CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
        }

        // On vide dans le bac
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        robots[no]->hold_items = 0;
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");

        // On retourne au parking
        char path3[MAX_WAYPOINTS][SIZE_POS];
        CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
        strcpy(current_pos,robots[no]->current_pos);
        sprintf(pos_finale, "P%d",(NB_COLONNES+1)*5 + no*(NB_COLONNES+1));
        CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");

        // On détermine la trajectoire
        trajectoire(current_pos, pos_finale, path3);

        // On boucle pour demander les mutex dans l'ordre
        for (int i = 0; i < MAX_WAYPOINTS; i++) {
            if (path3[i][0] == '\0') {
                break;
            }
            // On se met sur la file d'attente
            // Tant que ce n'est pas mon tour j'attend
            // TODO
 
            // C'est mon tour, je demande la mutex
            // TODO
            // On met à jour au fur et a mesure la liste des WAIPOINTS du robot
            CHECK(sem_wait(sem_memoire_robot[no]),"sem_wait(sem_memoire_robot)");
            add_waypoint(robots[no], path3[i]);
            // On met a jour la position du robot
            strcpy(robots[no]->current_pos,path3[i]);
            CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
        }
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
