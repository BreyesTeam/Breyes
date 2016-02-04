/*
** simulateur_client_arduino_to_proxy.c
** Br'Eyes project
** 25/01/2016

Ce client a pour but de simuler le client Arduino, vu qu'on ne peut pas toujours avoir la carte avec nous. 
Il y a deux champs à modifier
1) HOSTNAME : insérer ici le nom de l'@ IP publique sur laquelle le serveur est installé http://www.mon-ip.com/
La procédure pour installer le serveur Linux chez vous est décrite dans un autre fichier. 
Il est aussi possible de tester le proxy en local. 
C'est à dire que ce client est installé sur la même machine que le serveur, 
donc pas besoin de passer par le réseau internet. Il faut indiquer l'adresse locale : 127.0.0.1 dans HOSTNAME
2) request : c'est la chaîne de caractère que l'on envoie au proxy en fonction 
de l'état actuel sur Arduino  (par exemple on peut 
envoyer les coordonnées actuelles de l'Arduino, ou encore le numéro de bus sélectionné.

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
#define HOSTNAME "" //mettre l'addrese IP publique du serveur proxy ici http://www.mon-ip.com/
 
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
	char buf[MAXDATASIZE];
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

	state = stateONE;

	switch(state){
		case stateONE :
			request = "GPS[-0.54587748 ; 0.98474878]";
			break;
		case stateTWO : 
			request = "MORTDELOL";
			break;
		case stateTHREE : 
			request = "BUS_C4";
			break;
		case stateFOUR :
			request = "TRUC";
	}

        /* 4 : send the new request to proxy ---------------------------------*/
	if(send(sockfd, request, strlen(request), 0)==-1) perror("send");


	//* 5 : write the JSON code within a file
	output = fopen("monitor.txt", "a"); // Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.
 	if (output != NULL)
	{
		for(segment=0;((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) > 0);segment++){ 
		//les données arrives segmentées dû au mode de connexion TCP. On les recombines ici dans un fichier
			fputs(buf, output);
		}
		fputc('\0', output);	
	    	fclose(output);  
	}

	//* 6 Read our JSON file 
	output = fopen("monitor.txt", "r"); // Opens a file for reading. The file must exist.
 	if (output != NULL)
	{
		while((ch=fgetc(output)) != EOF ) printf("%c",ch);
		fclose(output);
	}
	printf("\n\n");
	close(sockfd);

	return EXIT_SUCCESS;
}
