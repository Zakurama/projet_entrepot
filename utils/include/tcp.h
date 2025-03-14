#ifndef _TCP_H
#define _TCP_H

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#define MAXOCTETS   500
#define MAXCLIENTS  100
#define DEFAULT_OK_MESSAGE "DONE"

void init_tcp_socket(int *sd, char *ip, u_int16_t port,int is_server);
int accept_client(int server_sd);
void listen_to(int se);
void send_message(int sd,char* message);
void recev_message(int sd,char *buff_reception);
void close_socket(int *se);

#endif
