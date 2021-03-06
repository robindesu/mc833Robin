/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
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
#include "headerMsg.h"
#include "dataAcces.c"
#include <sys/time.h>


#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define BUFFERSIZE 1024  // Bytes 


double time_diff(struct timeval x , struct timeval y)
{
    double x_ms , y_ms , diff;
     
    x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
    y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;
     
    diff = (double)y_ms - (double)x_ms;
     
    return diff;
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// Resp point to the response string created in request
char* receiveMsg(int new_fd){
	struct headerMsg hMsg;
	char buf[BUFFERSIZE];
	int numbytes;
	char *resp;

	numbytes = recv(new_fd, buf, BUFFERSIZE, 0);
	if (numbytes  == -1) {
        perror("recv");
        exit(1);
    }
	if(numbytes > 0){
		// printf("numbytes recived = %d\n", numbytes);

		memcpy(&hMsg, buf, sizeof(struct headerMsg));

		printf("Function name = %d\n", hMsg.functionName);
		printf("Payload Size = %d\n", hMsg.sizePayload);
		printf("Discipline = %s\n", hMsg.disciplineId);
		printf("Payload = %s\n", hMsg.disciplineId);
		resp = getRequest(hMsg);
		return resp;
	}
}

//Function to write a msg and send
void writeMsg(int new_fd, char* resp){
	int numbytes;
	if(strlen(resp) > BUFFERSIZE){
		printf("resposta maior que buffer\n");
	}
    numbytes = send(new_fd, resp, BUFFERSIZE, 0);
    if ( numbytes == -1)
        perror("send");
}
void executeRequests(int new_fd){
	double timesVetor[30];
	struct timeval before, after;
	char* resp;
	for(int i=0; i<30; i++){
        resp = receiveMsg(new_fd);
        gettimeofday(&before, NULL);
        writeMsg(new_fd, resp);	
        gettimeofday(&after, NULL);
        timesVetor[i]= time_diff(before , after) ; 
     }
     for(int i=0; i<30; i++){
		printf(" %.0lf  \n" , timesVetor[i]);
	}
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    // struct headerMsg hMsg;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections lol...\n");
 
    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            executeRequests(new_fd);
            exit(0);
        }
        close(new_fd);
        close(new_fd);  // parent doesn't need this
    }

    return 0;
}
