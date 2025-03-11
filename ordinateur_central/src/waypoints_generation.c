#include "waypoints_generation.h"


void name_waypoints_creation(Liste_pos_waypoints *liste_waypoints, int nb_column, int nb_row, int nb_robot) {
    int index = 0;
    char buffer[20];

    // Générer M(nb_column + 1) à M((nb_column+1)* nb_row*2) en incrémentant de nb_column +1
    for (int i = nb_column+1; i <=  (nb_column+1)* nb_row*2; i += nb_column+1) {
        snprintf(buffer, sizeof(buffer), "M%d", i);
        strcpy(liste_waypoints->name_waypoints[index++],buffer);
    }
    
    // Générer D(nb_column + 1) à D((nb_column+1)* nb_row*2) en incrémentant de nb_column +1
    for (int i = nb_column+1; i <=  (nb_column+1)* nb_row*2; i += nb_column+1) {
        snprintf(buffer, sizeof(buffer), "D%d", i);
        strcpy(liste_waypoints->name_waypoints[index++],buffer);
    }
    
    // Générer Sij pour i de 1 à nb_row et j de 1 à nb_column
    for (int i = 1; i <= nb_row; i++) {
        for (int j = 1; j <= nb_column; j++) {
            snprintf(buffer, sizeof(buffer), "S%d", 2*i*(nb_column+1)+j);
            strcpy(liste_waypoints->name_waypoints[index++],buffer);
        }
    }
    
    // Ajouter B(nb_column + 1) et B((nb_column+1) * 3)
    snprintf(buffer, sizeof(buffer), "B%d", (nb_column + 1));
    strcpy(liste_waypoints->name_waypoints[index++],buffer);
    snprintf(buffer, sizeof(buffer), "B%d", (nb_column+1) * 3);
    strcpy(liste_waypoints->name_waypoints[index++],buffer);
    
    // Générer Px parking
    for (int i = (nb_column+1) * 5 ; i < (nb_column+1) * 5 + (nb_column+1) * nb_robot; i +=  (nb_column+1)) {
        snprintf(buffer, sizeof(buffer), "P%d", i);
        strcpy(liste_waypoints->name_waypoints[index++],buffer);
    }
    
    // Ajouter un marqueur de fin
    strcpy(liste_waypoints->name_waypoints[index++],"\0");
}

void position_waypoints_creation(Liste_pos_waypoints *liste_waypoints, int nb_column, int nb_row, int nb_robot, Point hedge3, Point hedge4, Point hedge5) {
    int index = 0;
    
    // Calculs de base
    float y_M = hedge5.y - STORAGE_BIN_LENGTH - 2* ROBOT_WIDTH;
    float y_B = hedge4.y + ROBOT_WIDTH ;
    float y_D = (y_B+y_M)/2; 
    float x_0 = 50 + hedge3.x + (STORAGE_BIN_WIDTH / 2);

    // Placement des M*
    for (int i = 0; i < nb_row * 2 ; i++) {
        liste_waypoints->pos_waypoints[index].x = x_0 + i * (STORAGE_BIN_WIDTH + AISLE_WIDTH) / 2;
        liste_waypoints->pos_waypoints[index].y = y_M;
        index++;
    }

    // Placement des D*
    for (int i = 0; i < nb_row * 2 ; i++) {
        liste_waypoints->pos_waypoints[index].x = x_0 + i * (STORAGE_BIN_WIDTH + AISLE_WIDTH) / 2;
        liste_waypoints->pos_waypoints[index].y = y_D;
        index++;
    }

    // Placement des Sij
    for (int i = 0; i < nb_row; i++) {
        for (int j = 0; j < nb_column; j++) {
            liste_waypoints->pos_waypoints[index].x = liste_waypoints->pos_waypoints[1+i*2].x;
            liste_waypoints->pos_waypoints[index++].y = hedge5.y + STORAGE_BIN_LENGTH *j; 
        }
    }
    // Placement des B5 et B15 (même hauteur que M5 et M15)
    liste_waypoints->pos_waypoints[index].y = y_B;
    liste_waypoints->pos_waypoints[index++].x = liste_waypoints->pos_waypoints[0].x;// Même que M5
    liste_waypoints->pos_waypoints[index].y = y_B; 
    liste_waypoints->pos_waypoints[index++].x = liste_waypoints->pos_waypoints[2].x; // Même que M15

    // Placement des parkings (P25, P30, ...)
    for (int i = 0; i < nb_robot; i++) {
        liste_waypoints->pos_waypoints[index].y = y_B;
        liste_waypoints->pos_waypoints[index].x = liste_waypoints->pos_waypoints[4+i%nb_row].x;
        index++;
    }
}

void waypoints_creation (Liste_pos_waypoints liste_waypoints, Point hedge3, Point hedge4, Point hedge5, int nb_column, int nb_row, int nb_robot){
    name_waypoints_creation(&liste_waypoints,  nb_column,  nb_row,  nb_robot) ; 
    position_waypoints_creation(&liste_waypoints, nb_column, nb_row, nb_robot,   hedge3,  hedge4,  hedge5);
}
