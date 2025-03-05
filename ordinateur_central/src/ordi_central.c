#include "ordi_central.h"

void trajectoire(const char* pos_initiale, const char* pos_finale, char path[MAX_WAYPOINTS][SIZE_POS]){

    int pos_index = atoi(pos_initiale+1);
    char type_pos = pos_initiale[0];

    int pos_finale_index = atoi(pos_finale + 1);    
    char type_pos_finale = pos_finale[0];

    // On vérifie que les positions données sont cohérentes
    if(!(((type_pos=='S') && (pos_index%(NB_COLONNES + 1) != 0) && (pos_index % (2*(NB_COLONNES + 1))<(NB_COLONNES + 1)) ) || (type_pos!='S' && pos_index % (NB_COLONNES + 1) == 0))){
        printf("La position initiale donnée est non cohérente. Si de type S : ne doit pas être mutliple de %d, sinon doit être mutliple de %d\nSi de type S, il faut aussi que pos_initiale_index mod %d<%d\n",NB_COLONNES + 1,NB_COLONNES + 1,2*(NB_COLONNES + 1),NB_COLONNES + 1);
        exit(EXIT_FAILURE);
    }
    if(!(((type_pos_finale=='S') && (pos_finale_index%(NB_COLONNES + 1) != 0) && (pos_finale_index % (2*(NB_COLONNES + 1))<(NB_COLONNES + 1)) ) || (type_pos_finale!='S' && pos_finale_index%(NB_COLONNES + 1) == 0))){
        printf("La position finale donnée est non cohérente. Si de type S : ne doit pas être mutliple de %d, sinon doit être mutliple de %d\nSi de type S, il faut aussi que pos_finale_index mod %d<%d\n",NB_COLONNES + 1,NB_COLONNES + 1,2*(NB_COLONNES + 1),NB_COLONNES + 1);
        exit(EXIT_FAILURE);
    }            
    
    strcpy(path[0], pos_initiale);
    int i = 1;

    if(type_pos=='B'){
        // Si le début c'est un bac alors on retourne en PXX avant de repartir
        if(pos_index == 5){
            // On s'avance sur le deuxième bac
            strcpy(path[i], "B10");
            i++;
        }
        strcpy(path[i], "P25");
        i++;
        type_pos = 'P';
        pos_index = 25;
    }
    
    while (pos_index!=pos_finale_index || type_pos!=type_pos_finale){
        char holder_name_place[5];
        
        if(type_pos == 'P'){
            // On est au parking
            // On doit avancer sur la colone de montée
            sprintf(holder_name_place, "D%d", pos_index);
            strcpy(path[i], holder_name_place);
        }

        else if (pos_index > pos_finale_index){
            // On regarde ou on est
            if (type_pos == 'D'){
                // On est sur la colone de descente
                // On doit avancer sur la colone de montée
                sprintf(holder_name_place, "M%d", pos_index);
                strcpy(path[i], holder_name_place);
            }
           else if (type_pos == 'M'){
                // On est sur la colone de montée
                // On peut monter
                if(pos_index + NB_COLONNES > pos_finale_index){
                    sprintf(holder_name_place, "M%d", pos_index-(NB_COLONNES + 1));
                    strcpy(path[i], holder_name_place);
                }
            }
            else if (type_pos == 'S'){
                // On va vers la gauche
                if((pos_index)%(NB_COLONNES + 1)==0){
                    // On est arrivé en bout (début) des étagères
                    sprintf(holder_name_place, "M%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "S%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
            }
        }
                
        else if(pos_index +  NB_COLONNES >= pos_finale_index){
            // Je suis a la bonne altitude (on est devant la bonne étagère de stocks)
            if(pos_index < pos_finale_index && ((type_pos=='M')||(type_pos=='S'))){
                // On va vers la droite
                sprintf(holder_name_place, "S%d", pos_index+1);
                strcpy(path[i], holder_name_place);
            }
            // Je suis a la bonne altitude (on est devant le bon étage M)
            if(pos_index < pos_finale_index && ((type_pos=='D'))){
                // On va sur la colone de montée
                sprintf(holder_name_place, "M%d", pos_index);
                strcpy(path[i], holder_name_place);
            }
        }

        else if (pos_index < pos_finale_index){
            // On regarde ou on est
            if (type_pos == 'D'){
                // On est sur la colone de descente 
                // On peut descendre
                if(pos_index + NB_COLONNES +1<= pos_finale_index){
                    sprintf(holder_name_place, "D%d", pos_index+(NB_COLONNES + 1));
                    strcpy(path[i], holder_name_place);
                }
            }
            else if (type_pos == 'M'){
                // On est sur la colone de montée
                // On doit avancer sur la colone de montée
                sprintf(holder_name_place, "D%d", pos_index);
                strcpy(path[i], holder_name_place);
            }
            else if (type_pos == 'S'){
                // On va vers la gauche
                if((pos_index)%(NB_COLONNES + 1)==0){
                    // On est arrivé en bout (début) des étagères
                    sprintf(holder_name_place, "M%d", pos_index);
                    strcpy(path[i], holder_name_place);
                }
                else{
                    sprintf(holder_name_place, "S%d", pos_index-1);
                    strcpy(path[i], holder_name_place);
                }
            }

        }
        if(pos_index == pos_finale_index){
            if((type_pos=='D') && (type_pos_finale=='P')){
                // Je suis au bon niveau mais je veux aller au parking
                // On se déplace
                sprintf(holder_name_place, "P%d" ,pos_index);
                strcpy(path[i], holder_name_place);
            }
            if((type_pos=='M') && (type_pos_finale=='B')){
                // Je suis au bon niveau mais je veux aller sur les bacs
                // On se déplace
                sprintf(holder_name_place, "D%d" ,pos_index);
                strcpy(path[i], holder_name_place);
            }
            if((type_pos=='D') && (type_pos_finale=='B')){
                // Je suis au bon niveau mais je veux aller aux bacs
                // On se déplace
                sprintf(holder_name_place, "B%d" ,pos_index);
                strcpy(path[i], holder_name_place);
            }
        }

        pos_index = atoi(path[i]+1);
        type_pos = path[i][0];
        i++;
    }
}
