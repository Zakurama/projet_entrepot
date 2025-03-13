#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define CHECK_ERROR(val1, val2, msg) \
    if (val1 == val2) { perror(msg); exit(EXIT_FAILURE); }

#define MAXOCTETS 150

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <client_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sd;
    struct sockaddr_in adrlect, adrclient;
    socklen_t adrlect_len = sizeof(adrlect);
    int erreur, nbcar;
    char buff_emission[MAXOCTETS + 1];
    char buff_reception[MAXOCTETS + 1];

    // Create socket
    sd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_ERROR(sd, -1, "Erreur socket non cree !!!");

    printf("N° de la socket : %d\n", sd);

    // Set up client address (bind to specific IP)
    memset(&adrclient, 0, sizeof(adrclient));
    adrclient.sin_family = AF_INET;
    adrclient.sin_port = htons(0); // Let OS assign a random port
    adrclient.sin_addr.s_addr = inet_addr(argv[3]); // Client IP from CLI

    // Bind client socket
    erreur = bind(sd, (struct sockaddr *)&adrclient, sizeof(adrclient));
    CHECK_ERROR(erreur, -1, "Erreur lors du bind du client !!!");

    // Set up server address
    memset(&adrlect, 0, sizeof(adrlect));
    adrlect.sin_family = AF_INET;
    adrlect.sin_port = htons(atoi(argv[2])); // Server port from CLI
    adrlect.sin_addr.s_addr = inet_addr(argv[1]); // Server IP from CLI

    // Connect to the server
    erreur = connect(sd, (struct sockaddr *)&adrlect, adrlect_len);
    CHECK_ERROR(erreur, -1, "Erreur de connexion !!!");

    while (1) {

        // Receive message from server
        nbcar = recv(sd, buff_reception, MAXOCTETS, 0);
        CHECK_ERROR(nbcar, -1, "Problème de réception !!!");
        buff_reception[nbcar] = '\0';
        printf("MSG RECU DU SERVEUR : %s\n", buff_reception);
        
        // Check for exit command from server
        if (strcmp(buff_reception, "exit") == 0) {
            CHECK_ERROR(close(sd), -1, "Erreur lors de la fermeture de la socket");
            break;
        }

        // Send message to server
        printf("CLIENT> ");
        fgets(buff_emission, MAXOCTETS, stdin);
        buff_emission[strlen(buff_emission) - 1] = '\0'; // Remove newline
        nbcar = send(sd, buff_emission, strlen(buff_emission) + 1, 0);
        CHECK_ERROR(nbcar, 0, "Problème d'émission !!!");

        // Check for exit command
        if (strcmp(buff_emission, "exit") == 0) {
            CHECK_ERROR(close(sd), -1, "Erreur lors de la fermeture de la socket");
            break;
        }
    }

    CHECK_ERROR(close(sd), -1, "Erreur lors de la fermeture de la socket");

    exit(EXIT_SUCCESS);
}
