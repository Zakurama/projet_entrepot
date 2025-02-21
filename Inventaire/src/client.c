#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "utils.h"
#include "tcp.h"

int main(int argc, char *argv[]) {
    int sd;
    char buff_emission[MAXOCTETS+1];
    char buff_reception[MAXOCTETS+1];

    if (argc != 3){
        fprintf(stderr, "Usage: %s <Remote IP> <Remote Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char * remote_ip = argv[1];
    uint16_t remote_port = (u_int16_t) atoi(argv[2]);

    init_tcp_socket(&sd, remote_ip, remote_port,0);

     while (1) {

        // Envoi du message au serveur
        printf("Send your order (N_X.Y,...)\nN : number of articles, (X,Y) position in the stock (row,colon) \nCommand 'stock' : display the stocks\n-> ");
        fgets(buff_emission, MAXOCTETS, stdin);
        buff_emission[strlen(buff_emission) - 1] = '\0';
        send_message(sd,buff_emission);
    
        // Réception du message du serveur
        recev_message(sd,buff_reception);
        printf("Server sent: %s\n", buff_reception);
    }

}
