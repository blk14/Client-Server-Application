#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

using namespace std;

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

class Client {
  public:
	char input_buffer[61];
	char client_id[10];
};

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	Client client;
	

	FD_ZERO(&read_fds); // le initializati
    FD_ZERO(&tmp_fds);  // le initializati


	if (argc < 3) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");



	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	FD_SET(STDIN_FILENO, &read_fds);    // adaugati in read_fds standard inputul
    FD_SET(sockfd, &read_fds);      // adaugati in read_fds socketul folosit pentru comunicatia cu serverul

	char id[10];
	strcpy(id, argv[3]);
	ret = send(sockfd, id, sizeof(id), 0);
	DIE(ret < 0, "send");

	while (1) {

		tmp_fds = read_fds;

		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);   //multiplexam cu select

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {     // daca am primit ceva de la stdin
			// se citeste de la tastatura
			memset(client.input_buffer, 0, 61);
			fgets(client.input_buffer, sizeof(client.input_buffer), stdin);
			if (strncmp(client.input_buffer, "exit", 4) == 0) {
				break;
			}

			strcpy(client.client_id, argv[3]);
			// se trimite mesaj la server
			n = send(sockfd, &client, sizeof(client), 0);
			DIE(n < 0, "send");
		}

		if (FD_ISSET(sockfd, &tmp_fds)) {           // daca am primit ceva de la server (de pe socketul sockfd)
            memset(buffer, 0, BUFLEN);
            n = recv(sockfd, buffer, sizeof(buffer), 0);
            DIE(n < 0, "recv");
			if (strcmp(buffer, "exit") == 0) {
				break;
			}
            printf("%s\n", buffer);
        }
	}

	close(sockfd);

	return 0;
}