#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "LibSN.h"

/* 
 * Autore: Tommaso Rizzo - settembre 2018 
 */


void error(char *s){
    printf("%s", s);
    exit(1);
}

void error_t(char *s, int tid){
	printf("THREAD %d: ", tid);
    printf("%s", s);
    exit(1);
}

void get_input(char *buf) {
	memset(buf, 0, BUFLEN);      // inizializza il buf a 0
	fgets(buf, BUFLEN-1, stdin); // salva l'input nel buffer 
}

int receive_dim(int sock, int *dim) {
	int ret;
	
	ret = recv(sock, (void *) dim, sizeof(*dim), 0);
	if (ret < 0 || ret < sizeof(*dim)) {
		printf("Errore in lettura dal socket. Codice di ritorno %d.\n", ret);
		return 1;
	}
	printf("DEBUG: dim = %d\n", *dim);
	*dim = ntohl(*dim);
	printf("DEBUG: dim = %d\n", *dim);
	return 0;
}

int receive_msg(int sock, char *buf, int dim) {
	int ret;
	
	memset(buf, 0, strlen(buf));
	ret = recv(sock, (void *) buf, dim, 0);	
	if (ret < 0 || ret < dim) {
		printf("Errore in lettura dal socket. Codice di ritorno %d.\n", ret);
		return 1;
	}
	return 0;
}

int send_dim(int sock, int dim) {
    int ret;
	
	int d = htonl(dim);
    ret = send(sock, (void *) &d, sizeof(d), MSG_NOSIGNAL);
	printf("DEBUG: dim sent=%d\n", d);
    if (ret < 0 || ret < sizeof(d)){
        printf("Errore in lettura dal socket. Codice di ritorno %d.\n", ret);
        return 1;
    }
    return 0;
}

int send_msg(int sock, char *buf) {
    int ret;
	
    ret = send(sock, (void *) buf, strlen(buf), MSG_NOSIGNAL);
    printf("DEBUG: dim inviata=%d\n", (int)strlen(buf));
	if (ret < 0 || ret < strlen(buf)){
        printf("Errore in lettura dal socket. Codice di ritorno %d.\n", ret);
        return 1;
    }
	   return 0;
}
