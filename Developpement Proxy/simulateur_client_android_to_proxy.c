/*
** simulateur_client_android_to_proxy.c
** Br'Eyes project
** 25/01/2016

Ce client a pour but de simuler le client Android, au cas où (par exemple pour simuler 
le comportement que devrais avoir le mobile android).
Il envoie seulement son ID (voir tout en bas: ID_5).
Il y a deux champs à modifier:
1) HOSTNAME : insérer ici le nom de l'@ IP publique sur laquelle le serveur est installé http://www.mon-ip.com/ (pensez à ajouter une redirection d'adresse NAT dans le routeur). Pour tester en local (sans passer par internet), il suffit de mettre l'@ locale. 
La procédure pour installer le serveur Linux chez vous est décrite dans un autre fichier. 
2) request : c'est l'ID de la station android.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" //pas besoin de changer le PORT
#define MAXDATASIZE 2048 // max number of bytes we can get at once 
#define HOSTNAME "127.0.0.1" //mettre l'addrese IP publique du serveur proxy ici http://www.mon-ip.com/
 
enum state {stateONE, stateTWO, stateTHREE, stateFOUR};

// get sockaddr, IPv4:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE] = "";
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char ch;
	int segment=0;
	FILE* output = NULL;
	char *request;
	char state; 

	/* 1 : get the proxy information + socket creation -----------------------*/
  	printf("get server information...\n");

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(HOSTNAME, PORT, &hints, &servinfo)) != 0) {  // get sockaddr, IPv4
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	printf("file socket descriptor creation for web server...\n");
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

	/* 2 : proxy connection --------------------------*/
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	/* 3 : create the new request to send to the proxy----------------------*/	

	request = "ID_5";

        /* 4 : send the new request to proxy ---------------------------------*/
	if(send(sockfd, request, strlen(request), 0)==-1) perror("send");
	printf("request sent by android: %s\n", request);

	return EXIT_SUCCESS;
}
