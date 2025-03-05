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

void trajectoire(const char* pos_initiale, const char* pos_finale, char path[MAX_WAYPOINTS][SIZE_POS]);
void gestionnaire_inventaire(int se);
char *parse_client_request(const char *request, int max_elements, int L_n[max_elements], char *item_names[max_elements], int *count);
char *parse_stock(const char *request, int max_elements, int *L_n[max_elements], int *L_x[max_elements], int *L_y[max_elements], char *item_names[max_elements], int count[max_elements], int *nb_items_request);

#endif
