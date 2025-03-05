#include "tcp.h"
#include "utils.h"
#include "ordi_central.h"

#define IP_SIZE 16
#define DEFAULT_LOCALIP "127.0.0.1"
#define INVENTORY_IP "127.0.0.1"
#define INVENTORY_PORT 5000

int ports[NB_ROBOT] = {3000,8000};

char ip[IP_SIZE];

void bye();
void gestionnaire_inventaire(void);
void gestion_robot(int no);

typedef struct {
    int ID;
    int waypoints[MAX_WAYPOINTS];
    int liste_ID_articles[MAX_ARTICLES_LISTE_ATTENTE];
    int position_articles[MAX_ARTICLES_LISTE_ATTENTE];
} Robot;

int shm[NB_ROBOT];
size_t size_robot = sizeof(Robot);
Robot* robots[NB_ROBOT];

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
                gestionnaire_inventaire();
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
}

void gestionnaire_inventaire(void){
    // char* buffer_reception_ID_articles[50];
    // char* buffer_reception_pos_articles[50];
    // int se;
    // int ID_articles[MAX_ARTICLES_LISTE_ATTENTE];
    // int positions_possibles_articles[MAX_ARTICLES_LISTE_ATTENTE][MAX_ESPACE_STOCK];
    // int positions_choisies_articles[MAX_ARTICLES_LISTE_ATTENTE];
    // int ID_robot;
    // // Initialiser une connexion TCP avec l'inventaire
    // init_tcp_socket(&se,INVENTORY_IP,INVENTORY_PORT,0);
    while (1){

        // On attend une commande de l'inventaire
        // recev_message(se,buffer_reception_ID_articles); // la liste des articles (ID)
        // recev_message(se,buffer_reception_pos_articles); // la liste des positions

        // On extrait les articles demandés et leurs positions
        // TODO : Faire du parsing


        // On choisit le robot qui traitera la tâche et la position de l'article souhaité en stock
        // ID_robot = rand()%NB_ROBOT; // Pour l'instant pas de choix optimal du robot

        // On met à jour la liste des articles et la liste de position du robot
        // TODO

        // On informe l'inventaire qu'on a bien pris en compte sa demande (on indique quels articles de l'inventaire vont être pris)
        // TODO
    }
}

void gestion_robot(int no){
    int se;
    init_tcp_socket(&se,ip,ports[no],1);
    char buff_emission[MAX_WAYPOINTS];
    char buff_reception[50];
    while (1){
        int* waypoints = NULL;
        while(waypoints == NULL){
            // On attend
            sleep(2);
            CHECK(sem_wait(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
            waypoints = robots[no]->waypoints;
            CHECK(sem_post(sem_memoire_robot[no]),"sem_post(sem_memoire_robot)");
        }
        // Des waypoints ont été ajoutés
        // Traitons les
        send_message(se,buff_emission);
        recev_message(se,buff_reception);
        // A finir

    }
    close_socket(&se);
}
