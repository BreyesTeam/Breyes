/*
** simulateur_client_arduino_to_STAR
** Br'Eyes project
** 25/01/2016

Ce siumlateur est là pour tester un accès direct depuis Arduino vers le serveur STAR. Il suffit juste de modifier la 
constante URL pour obtenir la page web désiré. 

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

#define PORT "80" 
#define MAXDATASIZE 2048 // max number of bytes we can get at once 

#define HOSTNAME "data.explore.star.fr"
#define URL "/api/records/1.0/search/?dataset=tco-bus-topologie-pointsarret-td&facet=codeinseecommune&facet=nomcommune&facet=estaccessiblepmr&facet=mobilier"


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
	char *GETrequest = NULL;
	char request_to_web_server[1000];
	char ch;
	int segment=0;
	FILE* output = NULL;

	/* 1 : get web server information + socket creation -----------------------*/
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

	/* 2 : web server connexion --------------------------*/
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

	/* 3 : create the new request to send to the web server----------------------*/	
     	GETrequest = "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";			
   	sprintf(request_to_web_server, GETrequest, URL, HOSTNAME);
		
        printf("new built request: \n");
        printf("----------------------------------------------------------\n");
        printf("%s", request_to_web_server);
        printf("----------------------------------------------------------\n");
			
        /* 4 : send the new request to web server ---------------------------------*/
	if(send(sockfd, request_to_web_server, strlen(request_to_web_server), 0)==-1) perror("send");


	//* 5 : write the JSON code within a file
	output = fopen("monitor.txt", "a"); // Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.
 	if (output != NULL)
	{
		for(segment=0;((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) > 0);segment++){
	   		//printf("\n\n----------------------------------------------------------\n");
	    		//printf("segment %d\n", segment);
			fputs(buf, output);
  	    		//printf("----------------------------------------------------------\n");
	    		//buf[numbytes] = '\0';
	    		//printf("%s", buf);
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
