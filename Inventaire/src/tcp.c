#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "tcp.h"
#include "utils.h"

void init_tcp_socket(int *sd, char *ip, u_int16_t port,int is_server){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    *sd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_ERROR(*sd, -1, "Erreur socket non cree !!! \n");
    
    //preparation de l'adresse de la socket
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    
    if(is_server == 0){
        // Client
        int erreur = connect(*sd, (const struct sockaddr *)&addr, addr_len);
        CHECK_ERROR(erreur, -1, "Erreur de connexion !!!\n");
        printf("Connected to server %s:%d\n", ip, port);
    }
    else{
        // Server
        int erreur = bind(*sd, (const struct sockaddr *) &addr, addr_len);
        CHECK_ERROR(erreur, -1, "Erreur de bind !!! \n");
    }
}

int accept_client(int server_sd) {
    struct sockaddr_in adrclient;
    socklen_t adrclient_len = sizeof(adrclient);

    int client_sd = accept(server_sd, (struct sockaddr *)&adrclient, &adrclient_len);
    CHECK_ERROR(client_sd, -1, "Erreur de accept !!!\n");

    return client_sd;
}

void listen_to(int se){
    CHECK_ERROR(listen(se, MAXCLIENTS), -1, "Erreur de listen !!!\n");
}

void send_message(int sd,char* message){
    CHECK_ERROR(send(sd, message, strlen(message) + 1, 0), -1, "\nProblème d'émission !!!\n");
}

void recev_message(int sd,char *buff_reception){
    int nb_car = recv(sd, buff_reception, MAXOCTETS, 0);
    CHECK_ERROR(nb_car, -1, "\nProblème de réception !!!\n");
    if (nb_car == 0) { // Si le serveur se déconnecte
        exit(EXIT_SUCCESS);
    }
    buff_reception[nb_car] = '\0';
}

void close_socket(int se){
    CHECK_ERROR(close(se),-1,"Erreur lors de la fermeture de la socket\n");
}