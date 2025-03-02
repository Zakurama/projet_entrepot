#ifndef _ORDI_CENTRAL_H
#define _ORDI_CENTRAL_H

#define SIZE_POS 10
#define NB_ROBOT 2
#define NB_COLONNES 4
#define MAX_ARTICLES_PORTES 2
#define MAX_ARTICLES_LISTE_ATTENTE 10
#define MAX_ESPACE_STOCK 100
#define MAX_WAYPOINTS 100

void trajectoire(const char* pos_initiale, const char* pos_finale, char path[MAX_WAYPOINTS][SIZE_POS]);

#endif