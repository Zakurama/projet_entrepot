#ifndef _TCP_H
#define _TCP_H

#define MAXOCTETS   500
#define MAXCLIENTS  100

void init_tcp_socket(int *sd, char *ip, u_int16_t port,int is_server);
int accept_client(int server_sd);
void listen_to(int se);
void send_message(int sd,char* message);
void recev_message(int sd,char *buff_reception);
void close_socket(int *se);

#endif
