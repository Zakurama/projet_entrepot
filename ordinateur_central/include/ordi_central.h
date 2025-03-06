#ifndef _ORDI_CENTRAL_H
#define _ORDI_CENTRAL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <sys/mman.h>

#define SIZE_POS 10
#define NB_ROBOT 2
#define NB_COLONNES 4
#define NB_LIGNES 4
#define MAX_ARTICLES_PORTES 2
#define MAX_ARTICLES_LISTE_ATTENTE 10
#define MAX_ESPACE_STOCK 100
#define MAX_WAYPOINTS 100

typedef struct {
    char *item_name;
    int **positions; // Liste des coordonnées [x, y] du stock
    int *quantities; // Liste des quantités prélevées sur chaque position
    int count;       // Nombre de positions sélectionnées
} Item;

typedef struct {
    int ID;
    int waypoints[MAX_WAYPOINTS];
    
    char *item_name[MAX_WAYPOINTS];
    int *positions[MAX_WAYPOINTS];
    int *quantities[MAX_WAYPOINTS];

} Robot;

void trajectoire(const char* pos_initiale, const char* pos_finale, char path[MAX_WAYPOINTS][SIZE_POS]);
void gestionnaire_inventaire(int se);
char *parse_stock(const char *request, int max_elements, int *L_n[max_elements], int *L_x[max_elements], int *L_y[max_elements], char *item_names[max_elements], int count[max_elements], int *nb_items_request);
int choose_items_stocks(char *item_names_requested[], int L_n_requested[], int count_requested,char *item_names_stock[], int *L_n_stock[], int *L_x_stock[], int *L_y_stock[], int count_stock[],Item selected_items[]);
void update_shared_memory_stock(Robot *shared_memory,Item selected_items,int index_pos);
void receive_request_inventory(int client_sd,char *buffer_reception_ID_articles, char *buffer_reception_pos_articles,char *item_names_requested[MAX_ARTICLES_LISTE_ATTENTE],int L_n_requested[MAX_ARTICLES_LISTE_ATTENTE],int *L_n_stock[MAX_ARTICLES_LISTE_ATTENTE], int *L_x_stock[MAX_ARTICLES_LISTE_ATTENTE], int *L_y_stock[MAX_ARTICLES_LISTE_ATTENTE], char *item_names_stock[MAX_ARTICLES_LISTE_ATTENTE], int* count_requested, int count_stock[MAX_ARTICLES_LISTE_ATTENTE],int* nb_items);
void print_robot_state(Robot* robot);

#endif
