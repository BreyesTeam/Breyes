/*
  proxy.c - proxy server STAR - arduino/android program.
  Br'Eyes, 2016.
  Released into the INSA domain.
*********************************************************************************************
  Dans une première phase de test l'implémentation du proxy sera adaptée pour 
  un client Arduino seulement.
  Ce proxy utilise les socket Linux. Les sockets correspondent à l'interface entre 
  la couche application et la couche transport du model OSI. 
  Tout programme réseau doit donc utiliser un socket pour utiliser le réseau. 
  Les sockets possèdent plusieurs fonctions associées, notemment:
  - connect pour se connecter au serveur
  - listen pour écouter les requête des clients
  - send pour envoyer les données
  - recv pour recevoir
*********************************************************************************************  
  Ce proxy est configuré pour accéder au serveur web du STAR précisé dans la cste HOST. 
  Notre objectif est de se mettre en écoute (listen) sur un port donné (cste PORT, ici 3490), 
  de nous connecter au client (connect) 
  et de recevoir (recv) des données de la part du client (Arduino). 
  Le serveur STAR étant un server web, on se connectera par défaut en http (PORT 80).
  1) Ces données seront dans un premier temps des données GPS. 
  Le proxy va alors télécharger le fichier "topo des points d'arrêts", 
  en envoyant une requête bien précise au server web (connect puis send). 
  Le proxy fait ensuite le calcul de la distance euclidienne entre la position de L'arduino 
  et les arrêts de bus pour déterminer l'arrêt le + proche.
  2) Il télécharge ensuite le fichier "prochains passages des lignes de 
  bus à cet arrêt" (send puis recv). 
  Il filtre le contenu JSON. 
  Il envoie le résultat vers le client Arduino (send).
  3) Il se remet en écoute (listen) sur le port 3490  pour pouvoir réceptionner (recv) 
  le choix du bus de l'utilisateur.
  4) La suite...

  ---------------                 ---------------                ----------------
  -             -3490         3490-             -80            80-              -
  -   ARDUINO   -------------------    PROXY    ------------------ Serveur STAR -
  -             -     @IP Publique-             -@IP Publique    -              -
  ---------------                 ---------------                ----------------

  Certaine variables sont originaires du programme original. Elles seront supprimées à terme. 
*********************************************************************************************
  Pour compiler ce programme :
  ouvrir un terminal (CTRL+ALT+T), en se plaçant dans le répertoire courant du fichier proxy.c
    cd sousDossierOuJeVeuxAller
  Pour connaître les fichiers dans le répertoir courant et savoir où je suis :
    ls
  Puis exécuter la commande de compilation :
  gcc proxy.c -o serveurProxy
  Le premier argument proxy.c est le fichier à compiler, le deuxième argument 
  après -o est la sortie (o  comme output) et donc notre exécutable.
  On lance ensuite le programme :
    ./serveurProxy
*********************************************************************************************
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXDATASIZE 1460 // max number of bytes we can get at once
#define BACKLOG 10	 // how many pending connections queue will hold
#define PORT "3490" // the port users will be connecting to
#define HOST "data.explore.star.fr"

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/* -------------------------------------------------------------------------------*/
/* here is the algorithm followed in this c program ------------------------------*/
/* 
1: create a socket, binding the socket to the desired address and port
2: listen connection from a remote client host (arduino/android)
3: accept connections from the remote client host
4: receive data from the client
5: extract useful information
6: create the new request to send to the web server
7: get web server information + socket creation
8: client part proxy to web server connection
9 : send the new request to web server
10: send http data to the client + filter                                                                         
/* -------------------------------------------------------------------------------*/

int main(void)
{
  /* server proxy part variables */
  int sockfd;  // listen on sockfd
  struct addrinfo hints, *servinfo=NULL, *p=NULL /*used for socket, server part information*/;
  struct sockaddr_storage their_addr; // connector's address information
  char http_request[MAXDATASIZE];
  char s[INET6_ADDRSTRLEN];
  socklen_t sin_size; //used for accept
  struct sigaction sa;
  int yes=1;  
  int rv;

  /* client proxy part variables */
  int new_fd/*ClientSock*/, rv1, webServerSock;
  struct addrinfo *webServerInfo=NULL, *p1=NULL;
  struct addrinfo webServerHints;
  char *pch0=NULL, *pch1=NULL, start[MAXDATASIZE], *url=NULL, *host=NULL, *userAgent=NULL;
  char request_to_web_server[MAXDATASIZE], *request_base=NULL;
  int urlLen, hostLen, userLen, numbytes;

  /* http information get from web server */
  char received_data[MAXDATASIZE], header[MAXDATASIZE];
  int headerLen, segment=0;

  /* 1 : create a socket, binding the socket to the desired address and port */

  printf("get proxy server information...\n");

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){ // get sockaddr, IPv4 or IPv6:
    fprintf(stderr, "getaddrinfo proxy: %s\n", gai_strerror(rv));
    return EXIT_FAILURE;
  }

  // loop through all the results and bind to the first we can
  printf("file socket descriptor creation...\n");
  for(p = servinfo; p != NULL; p = p->ai_next){
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
      perror("proxy socket");
      continue;
    }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){ //configure the socket in order to bind
    perror("proxy setsockopt");
    exit(EXIT_FAILURE);
  }

  printf("binding...\n");
  if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){ //link socket with port serverProxy and ip serverProxy
    close(sockfd);
    perror("proxy bind");
    continue;
  }
  break;
  }

  if (p == NULL){
    fprintf(stderr, "proxy: failed to bind\n");
    return 2;
  }
  freeaddrinfo(servinfo); // all done with this structure


  /* 2 : listen  connection from a remote client host ----------------------------*/
  printf("----------------------------------------------------------\n");
  printf("listening on port %s...\n", PORT);
  if (listen(sockfd, BACKLOG) == -1){ //listen possible communications ; BACKLOG=maximum number of requests queuing (10)
    perror("listen");
    exit(EXIT_FAILURE);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1){
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  printf("waiting for connections...\n");	
  while(1){  // main accept() loop
    sin_size = sizeof their_addr;

    /* 3 : accept  connections from the remote client host ---------------------------*/

    new_fd = accept(sockfd/*serverProxy socket descriptor*/, (struct sockaddr *)&their_addr/*remote client host information*/, &sin_size); 
    //accept the communication with the browser and return a new socket file descriptor
    if (new_fd == -1){
      perror("accept");
      continue;
    }	   

    //network to string conversion (like network to ascii (inet_ntoa))
    inet_ntop(their_addr.ss_family/*source network adress*/, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    printf("got connection from %s\n", s);

    /* forking to handle concurrent connections ------------*/
    if (!fork()) { // this is the child process
      close(sockfd); // child doesn't need the listener

      /* 4 : receive data from the client  ----------------------------------------------*/

      if ((numbytes = recv(new_fd, http_request, MAXDATASIZE-1, 0)) == -1){
        perror("recv");
      }
      http_request[numbytes] = '\0';
      printf("originale request : \n");
      printf("----------------------------------------------------------\n");
      printf("%s", http_request);


      /* 5 : extract useful information ------------------------------------------------*/

      // get the url from the original request
      //pch0 = strstr(http_request, "GET http://");
      //strcpy(start, pch0+11+strlen(host)); //  = sizeof "GET http:// + sizeof host" 
      //pch1 = strstr(http_request, " HTTP");
      //urlLen = strlen(start) - strlen(pch1);
      //if((url = (char*)calloc(urlLen+1,sizeof(char))) == NULL) 
      //fprintf(stderr, "start variable allocation failure\n");
      //strncpy(url, start, urlLen);

      /* 6 : create the new request to send to the web server----------------------*/

      request_base = "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n";		
      sprintf(request_to_web_server, request_base, url, host, userAgent);

      printf("new built request : \n");
      printf("----------------------------------------------------------\n");
      printf("%s", request_to_web_server);
      printf("----------------------------------------------------------\n");

      printf("get web server information...\n");

      /* 7 : get web server information + socket creation -----------------------*/

      webServerHints.ai_family = AF_UNSPEC;
      webServerHints.ai_socktype = SOCK_STREAM;
      if ((rv1 = getaddrinfo(HOST,"80", &webServerHints, &webServerInfo)) != 0) { // get sockaddr, IPv4 or IPv6:
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv1));
        exit(EXIT_FAILURE);
      }

      printf("file socket descriptor creation for web server...\n");
      for(p1 = webServerInfo; p1 != NULL; p1 = p1->ai_next) {
        if ((webServerSock = socket(p1->ai_family, p1->ai_socktype,   /*get the file descriptor : socket()*/ p1->ai_protocol)) == -1) {
          perror("web server: socket");
          continue;
        }
        break;
      }

      /* 8 : client part proxy to web server connection --------------------------*/

      printf("connexion to %s\n", HOST);
      if (connect(webServerSock, p1->ai_addr/*remote adress and port*/, p1->ai_addrlen) == -1) {
        close(sockfd);
        perror("client part : connexion to the web server failure");
        continue;
      }

      /* 9 : send the new request to web server ---------------------------------*/

      if(send(webServerSock, request_to_web_server, strlen(request_to_web_server), 0)==-1) perror("send");

      /* 10 : send http data to the arduino board + filter -----------------------------*/

      for(segment=0;((numbytes = recv(webServerSock, received_data, MAXDATASIZE, 0)) > 0);segment++){
        if(!segment){ // if we are reading the header
          pch1 = strstr(received_data, "\r\n\r\n");	
          headerLen = strlen(received_data) - strlen(pch1);
          strncpy(header, received_data, headerLen);
          printf("header : \n%s", header);
          printf("\n----------------------------------------------------------\n");
        }	  
      }

      send(new_fd,received_data,numbytes,0);
      printf("send information got from the web server to client (segment %d):\n", segment);
      printf("----------------------------------------------------------\n");
      printf("%s", received_data);
      printf("----------------------------------------------------------\n");	  
      free(host);
      freeaddrinfo(webServerInfo); 
      exit(EXIT_SUCCESS);
    }
    close(new_fd);
  }		     
  free(start);
  free(request_to_web_server);
  free(received_data);

  return EXIT_SUCCESS;
}
