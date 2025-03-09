#ifndef _WAYPOINTS_GEN_H
#define _WAYPOINTS_GEN_H
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>


/**  moins devant x pour avoir un repère orthonormé "correct" pour les robots 
 * Valeur en millimètre pour harmoniser avec le code des robots
*/

#define DEFAULT_HEDGE_2 (Point){0.0f, 0.0f} // x : mm, y : mm
#define DEFAULT_HEDGE_3 (Point){-1781.0f, 0.0f} // x : mm, y : mm
#define DEFAULT_HEDGE_4 (Point){-896.0f, -1107.0f} // x : mm, y : mm
#define DEFAULT_HEDGE_5 (Point){-882.0f, 1103.0f} // x : mm, y : mm

#define DEPOSIT_BIN_WIDTH 300 // mm

#define ROBOT_WIDTH 200 // mm
#define PARKING_WIDTH 400

#define STORAGE_BIN_LENGTH 400 // mm
#define STORAGE_BIN_WIDTH 250 // mm (Il faut que le robot puisse attendre devant)

#define AISLE_WIDTH 300 // mm

#define MAX_WAYPOINTS 100

typedef struct {
    float x; 
    float y; 
} Point; 

typedef struct {
    Point pos_waypoints[MAX_WAYPOINTS]; 
    char * name_waypoints[MAX_WAYPOINTS]; 
} Liste_pos_waypoints; 

void name_waypoints_creation(Liste_pos_waypoints *liste_waypoints, int nb_column, int nb_row, int nb_robot) ;
void position_waypoints_creation(Liste_pos_waypoints *liste_waypoints, int nb_column, int nb_row, int nb_robot, Point hedge3, Point hedge4, Point hedge5) ; 
void waypoints_creation (Liste_pos_waypoints *liste_waypoints, Point hedge3, Point hedge4, Point hedge5, int nb_column, int nb_row, int nb_robot); 