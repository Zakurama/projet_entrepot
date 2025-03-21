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

#include "waypoints_generation.h"
#include "tcp.h"

#define NB_MAX_ROBOT 10
#define DEFAULT_NB_COLONNES 4
#define DEFAULT_NB_LIGNES 4
#define DEFAULT_NB_BACS 2
#define NB_MAX_LIGNES 8
#define NB_MAX_COLONNES 8
#define NB_MAX_BAC 2
#define NAME_ITEM_SIZE 10
#define MAX_ARTICLES_PORTES 2
#define MAX_ARTICLES_LISTE_ATTENTE 10
#define MAX_ESPACE_STOCK 100

extern int *nb_colonnes;
extern int *nb_lignes;

typedef struct {
    char *item_name;
    int **positions; // Liste des coordonnées [x, y] du stock
    int *quantities; // Liste des quantités prélevées sur chaque position
    int count;       // Nombre de positions sélectionnées
} Item_selected;

typedef struct {
    int ID;

    char current_pos[SIZE_POS];
    int hold_items;

    char waypoints[MAX_WAYPOINTS][SIZE_POS];
    Point * pos_waypoints[MAX_WAYPOINTS]; 

    char item_name[MAX_WAYPOINTS][NAME_ITEM_SIZE];
    int positions[MAX_WAYPOINTS];
    int quantities[MAX_WAYPOINTS];

} Robot;

void trajectoire(const char* pos_initiale, const char* pos_finale, char path[MAX_WAYPOINTS][SIZE_POS], int nb_lignes, int nb_colonnes);
char *parse_stock(const char *request, int max_elements, int *L_n[max_elements], int *L_x[max_elements], int *L_y[max_elements], char *item_names[max_elements], int count[max_elements], int *nb_items_request);
int choose_items_stocks(char *item_names_requested[], int L_n_requested[], int count_requested,char *item_names_stock[], int *L_n_stock[], int *L_x_stock[], int *L_y_stock[], int count_stock[],Item_selected selected_items[]);
int update_shared_memory_stock(Robot *robot, Item_selected selected_item, int index_pos,int nb_colonnes);
char* convert_request_strings_to_lists(char *buffer_reception_ID_articles, char *buffer_reception_pos_articles,char *item_names_requested[MAX_ARTICLES_LISTE_ATTENTE],int L_n_requested[MAX_ARTICLES_LISTE_ATTENTE],int *L_n_stock[MAX_ARTICLES_LISTE_ATTENTE], int *L_x_stock[MAX_ARTICLES_LISTE_ATTENTE], int *L_y_stock[MAX_ARTICLES_LISTE_ATTENTE], char *item_names_stock[MAX_ARTICLES_LISTE_ATTENTE], int* count_requested, int count_stock[MAX_ARTICLES_LISTE_ATTENTE],int* nb_items);
void print_robot_state(Robot* robot);
void convert_items_to_lists(Item_selected *selected_items, int num_items,char *chosen_item_names[MAX_ARTICLES_LISTE_ATTENTE],int *chosen_x_positions[MAX_ARTICLES_LISTE_ATTENTE],int *chosen_y_positions[MAX_ARTICLES_LISTE_ATTENTE],int *chosen_quantities[MAX_ARTICLES_LISTE_ATTENTE],int chosen_counts[MAX_ARTICLES_LISTE_ATTENTE]);
char *create_inventory_string(int nb_items, int max_elements, int count[max_elements], int *L_n[max_elements], int *L_x[max_elements], int *L_y[max_elements], char *item_names[max_elements]);
int authorize_robot_connexion(char *file_name, char *robot_ip);
void gestion_flotte(int *nb_robots, char *ip);

int remove_first_waypoint_of_robot(Robot *robot);
int add_waypoint(Robot *robot, const char *waypoint);
void generate_waypoints(const char current_pos[SIZE_POS],const char pos_finale[SIZE_POS],Robot* memoire_robot, sem_t* sem_robot,sem_t* sem_bac[NB_MAX_BAC],sem_t* sem_parking[NB_MAX_ROBOT],sem_t* sem_lignes[NB_MAX_LIGNES],sem_t* sem_colonneNord[2*NB_MAX_LIGNES],sem_t* sem_colonneSud[2*NB_MAX_LIGNES],int nb_lignes, int nb_colonnes,int nb_bac);
int remove_first_item_of_robot(Robot *robot);
void get_current_and_final_pos(Robot* robot,int no,char current_pos[SIZE_POS],char pos_finale[SIZE_POS],char type_final_pos,int nb_colonnes,int nb_bac);
int get_index_of_waypoint(char type_pos,int no_pos,int nb_colonnes,int nb_bac);
void free_mutex(char type_pos,int no_mutex,sem_t* sem_bac[NB_MAX_BAC],sem_t* sem_parking[NB_MAX_ROBOT],sem_t* sem_lignes[NB_MAX_LIGNES],sem_t* sem_colonneNord[2*NB_MAX_LIGNES],sem_t* sem_colonneSud[2*NB_MAX_LIGNES]);
void generer_trame_robot_waypoints(char buffer[MAXOCTETS],char waypoints[MAX_WAYPOINTS][SIZE_POS], Liste_pos_waypoints * liste_waypoints);

#endif
