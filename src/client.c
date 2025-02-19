#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <asm-generic/signal-defs.h>
#include <bits/sigaction.h>

#define CHECK_ERROR(val1,val2,msg)   if (val1==val2) \
                                    { perror(msg); \
                                        exit(EXIT_FAILURE); }

#define MAXOCTETS   150

void init_tcp_socket(int *sd, char *remote_ip, u_int16_t remote_port){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    *sd = socket(AF_INET, SOCK_DGRAM, 0);
    CHECK_ERROR(*sd, -1, "Erreur socket non cree !!! \n");
    
    //preparation de l'adresse de la socket
    addr.sin_family = AF_INET;
    addr.sin_port = htons(remote_port);
    addr.sin_addr.s_addr = inet_addr(remote_ip);
    
    //Affectation d'une adresse a la socket
    int erreur = bind(*sd, (const struct sockaddr *) &addr, addr_len);
    CHECK_ERROR(erreur, -1, "Erreur de bind !!! \n");
    printf("Waiting for request...\n");
}

void handle_communication(int sd){
    char buff_emission[MAXOCTETS+1];
    char buff_reception[MAXOCTETS+1];
    int nb_car;

    // Envoi du message au serveur
    printf("Send your order (N_X.Y,...)\n-> ");
    fgets(buff_emission, MAXOCTETS, stdin);
    buff_emission[strlen(buff_emission) - 1] = '\0';
    nb_car = send(sd, buff_emission, strlen(buff_emission) + 1, 0);
    CHECK_ERROR(nb_car, 0, "\nProblème d'émission !!!\n");

    // Réception du message du serveur
    nb_car = recv(sd, buff_reception, MAXOCTETS, 0);
    CHECK_ERROR(nb_car, -1, "\nProblème de réception !!!\n");
    buff_reception[nb_car] = '\0';
    printf("Server sent: %s\n", buff_reception);
    // need message handling there to print failure or success
}

int main(int argc, char *argv[]) {
    int sd;
    
    if (argc != 3){
        fprintf(stderr, "Usage: %s <Remote IP> <Remote Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char * remote_ip = argv[1];
    uint16_t remote_port = (u_int16_t) atoi(argv[2]);

    init_tcp_socket(&sd, remote_ip, remote_port);

     while (1) {
        handle_communication(sd);
    }

}
