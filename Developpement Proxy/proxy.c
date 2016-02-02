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

#define MAXDATASIZE 8192 // max number of bytes we can get at once (admit by most of browsers)
#define SIZEOFBADWORD 20
#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/* -------------------------------------------------------------------------------*/
/* here is the algorithm followed in this c program ------------------------------*/
/* 1: black list management
   2: creating a socket, binding the socket to the desired address and port
   3: listen connexion from a remote client host
   4: accept connexions from a remote client host
   5: receive data from browser
   6: extract useful headers
   7: create the new request to send to the web server
   8: get web server information + socket creation
   9: client part proxy to web server connexion
   10: filter url
   11 : send the new request to web server
   12: send http data to the browser + filter                                                                         
   /* -------------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  // The command line argument must contain the port number
  assert(argc == 2);
  char *port = argv[1];

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
 
  /* filter variables */
  int badWord=0; //boolean
  int filter=0; //boolean
  int i, c, nbBadWord;
  char **t_badWord=NULL, *ptrEndLine = NULL;
  FILE *blackList = NULL;

  /* http information get from web server */
  char received_data[MAXDATASIZE], header[MAXDATASIZE];
  int headerLen, segment=0;

  /* 1 : black list management ----------------------------------------------*/
	      
  printf("---------------------------------------------------------\n");
  if ((blackList = fopen("blacklist.txt","r")) == NULL)
    {
      fprintf(stderr,"blacklist.txt file opening failure \n");
      exit(EXIT_FAILURE);
    }

  while ((c = fgetc(blackList)) != EOF) if (c=='\n') nbBadWord++;  
  if((t_badWord = (char**)malloc(nbBadWord*sizeof(*t_badWord))) == NULL) 
    fprintf(stderr, "t_badWord variable allocation failure\n");
  for(i=0;i<nbBadWord;i++){ //calculate the number of bad words
    if((t_badWord[i]=(char*)malloc(SIZEOFBADWORD*sizeof(**t_badWord))) == NULL) 
      fprintf(stderr, "start variable allocation failure\n"); 
  }
			
  rewind(blackList);			
  printf("blacklist.txt :\n");
  for(i=0;i<nbBadWord;i++){
    fgets(t_badWord[i],SIZEOFBADWORD,blackList); // fetch each line (each bad word) of the black list text file
    ptrEndLine = strchr(t_badWord[i], '\n');
    if(ptrEndLine) *ptrEndLine='\0'; // erase \n
    printf("%s\n",t_badWord[i]);
  }
		
  if(fclose(blackList) == EOF){
    fprintf(stderr,"file closing failure\n");
    exit(EXIT_FAILURE);
  }

  printf("----------------------------------------------------------\n");

  /* 2 : creating a socket, binding the socket to the desired address and port */

  printf("get proxy server information...\n");

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0){ // get sockaddr, IPv4 or IPv6:
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


  /* 3 : listen  connexion from a remote client host ----------------------------*/
  printf("----------------------------------------------------------\n");
  printf("listening on port %s...\n", port);
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

    /* 4 : accept  connexions from a remote client host ---------------------------*/

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

      /* 5 : receive data from browser ----------------------------------------------*/

      if ((numbytes = recv(new_fd, http_request, MAXDATASIZE-1, 0)) == -1){
	perror("recv");
      }
      http_request[numbytes] = '\0';
      printf("originale request : \n");
      printf("----------------------------------------------------------\n");
      printf("%s", http_request);
	

      /* 6 : extract useful headers ------------------------------------------------*/
      	
      // get host from the original request
      pch0 = strstr(http_request, "Host: "); //another start string pointer
      strcpy(start, pch0+6); // 6 = sizeof "Host: " 
      pch1 = strstr(start, "\r"); //end string pointer
      hostLen = strlen(start) - strlen(pch1);
      if ((host = (char*)calloc(hostLen+1, sizeof(char))) == NULL) 
	fprintf(stderr, "host variable allocation failure\n");
      strncpy(host, start, hostLen);

      // get the url from the original request
      pch0 = strstr(http_request, "GET http://");
      strcpy(start, pch0+11+strlen(host)); //  = sizeof "GET http:// + sizeof host" 
      pch1 = strstr(http_request, " HTTP");
      urlLen = strlen(start) - strlen(pch1);
      if((url = (char*)calloc(urlLen+1,sizeof(char))) == NULL) 
	fprintf(stderr, "start variable allocation failure\n");
      strncpy(url, start, urlLen);

      // get user-agent from the original request
      pch0 = strstr(http_request, "User-Agent: ");
      strcpy(start, pch0+12); // 12 = sizeof "User-Agent: "
      pch1 = strstr(start, "\r");
      userLen = strlen(start) - strlen(pch1);
      if((userAgent = (char*)calloc(userLen+1, sizeof(char))) == NULL) 
	fprintf(stderr, "userAgent variable allocation failure\n");
      strncpy(userAgent, start, userLen);   	

      /* 7 : create the new request to send to the web server----------------------*/
         
      if(url[0]=='/') request_base = "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n";		
      else request_base = "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n";		
      sprintf(request_to_web_server, request_base, url, host, userAgent);
		
      printf("new built request : \n");
      printf("----------------------------------------------------------\n");
      printf("%s", request_to_web_server);
      printf("----------------------------------------------------------\n");
			
      printf("get web server information...\n");

      /* 8 : get web server information + socket creation -----------------------*/

      webServerHints.ai_family = AF_UNSPEC;
      webServerHints.ai_socktype = SOCK_STREAM;
      if ((rv1 = getaddrinfo(host,"80", &webServerHints, &webServerInfo)) != 0) { // get sockaddr, IPv4 or IPv6:
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

      /* 9 : client part proxy to web server connexion --------------------------*/

      printf("connexion to %s\n", host);
      if (connect(webServerSock, p1->ai_addr/*remote adress and port*/, p1->ai_addrlen) == -1) {
	close(sockfd);
	perror("client part : connexion to the web server failure");
	continue;
      }

      /* 10 : filter url ---------------------------------------------------------*/

      for(i=0;i<nbBadWord;i++){ //looking for bad word in url
	if(strstr(url, t_badWord[i])!=NULL){
	  printf("\nbad word found in URL : %s\n", t_badWord[i]);
	  badWord=1;
	}
      }

      /* 11 : send the new request to web server ---------------------------------*/

      if(!badWord){
	if(send(webServerSock, request_to_web_server, strlen(request_to_web_server), 0)==-1)
	  perror("send");
      }
	     		
      /* 12 : send http data to the browser + filter -----------------------------*/
	
      if(badWord){ //if a bad word is present in url
	badWord=0;
	send(new_fd,"HTTP/1.1 302 Found\r\nLocation: http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error1.html\r\n\r\n",100,0);
	printf("HTTP/1.1 302 Found\n");
      } 
      else{ // if there are no bad words in url		
	for(segment=0;((numbytes = recv(webServerSock, received_data, MAXDATASIZE, 0)) > 0) && !badWord;segment++){
	  if(!segment){ // if we are reading the header
	    pch1 = strstr(received_data, "\r\n\r\n");	
	    headerLen = strlen(received_data) - strlen(pch1);
	    strncpy(header, received_data, headerLen);
	    printf("header : \n%s", header);
	    printf("\n----------------------------------------------------------\n");
	    if((strstr(header, "gzip") == NULL) && (strstr(header, "text/") != NULL)) filter=1;
	  }	  
	  // if there are no gzip encoded content and if there are text content
	  if(filter){ // we can filter it	    
	    for(i=0;i<nbBadWord;i++){ //looking for bad word in text content
	      if(strstr(received_data, t_badWord[i])!=NULL){ //if a badword if found it content	
		printf("\nbad word found in content : %s\n", t_badWord[i]);	        
		badWord=1;
	      }
	    }
	    
	    if(badWord){ //if a bad word is found in content	      
	      printf("HTTP/1.1 302 Found\n");
	      send(new_fd,"HTTP/1.1 302 Found\r\nLocation: http://www.ida.liu.se/~TDTS04/labs/2011/ass2/error2.html\r\n\r\n",100,0);
	    }
	    else{ // if there are no bad words
	      send(new_fd,received_data,numbytes,0);
	      printf("send information got from the web server to client (segment %d):\n", segment);
	      printf("----------------------------------------------------------\n");
	      printf("%s", received_data);
	    }
	  }
	  else{ // gzip content or no text
	    printf("Dont filter !\n");				    
	    send(new_fd,received_data,numbytes,0);
	    printf("send information got from the web server to client (segment %d):\n", segment);
	    printf("----------------------------------------------------------\n");
	    printf("[compressed/binary data]\n");
	  }
	  printf("----------------------------------------------------------\n");	  
	}
	badWord=0;
      }	  
      filter=0;      
      free(url);free(host);free(userAgent);
      freeaddrinfo(webServerInfo); 
      exit(EXIT_SUCCESS);
    }
    close(new_fd);
  }		     
  free(start);
  for(i=0;i<nbBadWord;i++) free(t_badWord[i]);
  free(t_badWord);
  free(request_to_web_server);
  free(received_data);

  return EXIT_SUCCESS;
}
