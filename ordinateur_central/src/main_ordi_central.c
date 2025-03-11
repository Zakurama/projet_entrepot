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

int shm[NB_ROBOT];
size_t size_robot = sizeof(Robot);
Robot* robots[NB_ROBOT];

int shm_liste_waypoints;
size_t size_liste_waypoints = sizeof(Liste_pos_waypoints);
Liste_pos_waypoints * liste_waypoints; 

sem_t* sem_memoire_robot[NB_ROBOT];

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

    // Création de la mémoire partagée pour la liste de waypoints

    CHECK(shm_liste_waypoints = shm_open("liste_waypoints", O_CREAT | O_RDWR, 0666),"shm_open(shm_liste_waypoints)");
    CHECK(ftruncate(shm_liste_waypoints, size_liste_waypoints),"ftruncate(shm_liste_waypoints)");
    CHECK_MAP(liste_waypoints = mmap(0, size_liste_waypoints, PROT_READ | PROT_WRITE, MAP_SHARED, shm_liste_waypoints, 0),"mmap");

    waypoints_creation(*liste_waypoints, DEFAULT_HEDGE_3, DEFAULT_HEDGE_4, DEFAULT_HEDGE_5, nb_colonnes, nb_lignes, NB_ROBOT);

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
                while(1){
                    gestionnaire_inventaire(client_sd);
                }
            }
            if(i>0 && i<=NB_ROBOT){
                // Processus de gestion des robots
                printf("Robot %d\n",i-1);
                gestion_robot(i-1);
            }
            else if(i>NB_ROBOT && i< nb_processus){
                // Autres gestionnaires
                // Pour l'instant rien
                printf("Gestionnaire de traj de robot %d\n",i-NB_ROBOT-1);
                while(1);
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

    CHECK(munmap(liste_waypoints, size_liste_waypoints),"munmap(robots_data)");
    CHECK(close(shm_liste_waypoints),"close(shm_liste_waypoints)");
    CHECK(shm_unlink("liste_waypoints"),"shm_unlink(liste_waypoints)");

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

    // On récupère les demandes de l'inventaire
    recev_message(client_sd, buffer_reception_ID_articles); // la liste des articles (ID)

    // à ce moment soit l'inventaire à transmit une commande de clients 
    // soit il a transmit le nombre de lignes ou de colonnes de l'inventaire
    
    // On vérifie si l'inventaire a envoyé une commande de modification de la taille de l'inventaire
    int new_size = 0;
    char size_type[MAXOCTETS];
    if (sscanf(buffer_reception_ID_articles, "%[^,],%d", size_type, &new_size) == 2) {
        if (strcmp(size_type, "rows") == 0) {
            // Modification du nombre de lignes
            nb_lignes = new_size;
            waypoints_creation (*liste_waypoints, DEFAULT_HEDGE_3, DEFAULT_HEDGE_4, DEFAULT_HEDGE_5, nb_colonnes, nb_lignes, NB_ROBOT);
        } else if (strcmp(size_type, "columns") == 0) {
            // Modification du nombre de colonnes
            nb_colonnes = new_size;
            waypoints_creation (*liste_waypoints, DEFAULT_HEDGE_3, DEFAULT_HEDGE_4, DEFAULT_HEDGE_5, nb_colonnes, nb_lignes, NB_ROBOT);
        }
        else {
            strcpy(buffer_emission, "Invalid size type");
            send_message(client_sd, buffer_emission);
            return;
        }
        strcpy(buffer_emission, "Size updated successfully");
        send_message(client_sd, buffer_emission);
        return;
    }

    // L'inventaire à envoyé une requête de commande
    // On récupère les positions des articles en stock
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
    int ID_robot = 0;
    for (int i = 0; i < nb_selected; i++) {
        for (int j = 0;j<selected_items[i].count;j++){
            // On met à jour sa mémoire partagée
            CHECK(sem_wait(sem_memoire_robot[ID_robot]),"sem_wait(sem_memoire_robot)");
            update_shared_memory_stock(robots[ID_robot],selected_items[i],j);
            CHECK(sem_post(sem_memoire_robot[ID_robot]),"sem_post(sem_memoire_robot)");
            ID_robot = (ID_robot + 1) % NB_ROBOT; // Pour l'instant pas de choix optimal du robot, on prend juste à son tour les robots
        }
    }

    for(int i = 0;i<NB_ROBOT;i++){
        print_robot_state(robots[i]);
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

void gestion_robot(int no){
    while(1);
    // int se;
    // init_tcp_socket(&se,ip,ports[no],1);
    // char buff_emission[MAX_WAYPOINTS];
    // char buff_reception[50];
    // while (1){
    //     int* waypoints = NULL;
    //     while(waypoints == NULL){
    //         // On attend
    //         sleep(2);
    //         CHECK(sem_wait(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
    //         waypoints = robots[no]->waypoints;
    //         CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
    //     }
    //     // Des waypoints ont été ajoutés
    //     // Traitons les
    //     send_message(se,buff_emission);
    //     recev_message(se,buff_reception);
    //     // A finir

    // }
    // close_socket(&se);
}
