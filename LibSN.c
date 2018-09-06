#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "LibSN.h"

void error(char *s){
    printf("%s", s);
    exit(1);
}

void get_input(char *buf) {
	memset(buf, 0, BUFLEN);      // inizializza il buf a 0
	fgets(buf, BUFLEN-1, stdin); // salva l'input nel buffer 
}

int receive_msg(int sock, char *buf) {
	int ret;
	
	memset(buf, 0, BUFLEN);
	ret = recv(sock, (void *) buf, BUFLEN, 0);	
	if (ret) {
		printf("Errore in lettura dal socket. Codice di ritorno %d.\n", ret);
		return 1;
	}
	return 0;
}

int send_msg(int sock, char *buf) {
    int ret;
	
    ret = send(sock, (void *) buf, BUFLEN, MSG_NOSIGNAL);
    if (ret < BUFLEN){
        printf("Errore in lettura dal socket. Codice di ritorno %d.\n", ret);
        return 1;
    }
    return 0;
}
