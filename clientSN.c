#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* 
 * Autore: Tommaso Rizzo - settembre 2018 
 */
 
// dichiarazione funzioni
int client_connect(int port, char *ip_addr);
 
int main (int argc, char *argv[]) {
	
	int sock, port;
	char *ip_addr;
	
	if (argc != 3) {
		printf("Errore, inserire: ./sn_client <host remoto> <porta>\n");
		exit(1);
	}
	
	ip_addr = argv[1];
	port    = atoi(argv[2]);
	
	// inizializzazione e connessione al server
	sock = client_connect(port, ip_addr);
	
	// terminazione
	close(sock);
    printf("Connessione al server terminata correttamente. Uscita.\n");
    return 0;
}

int client_connect(int port, char *ip_addr) {
	
	struct sockaddr_in srv_addr;
    int sk;

	sk = socket(AF_INET, SOCK_STREAM, 0);
	if (sk < 0)  {
		perror("Errore nell'apertura del socket.\n");
		exit(sk);
	}
	
    memset(&srv_addr, 0, sizeof(srv_addr));          // inizializzazione
    srv_addr.sin_family =     AF_INET;				 // IPv4
    srv_addr.sin_port   = htons(port);				 // porta
 	inet_pton(AF_INET, ip_addr, &srv_addr.sin_addr); // inidirizzo IP
	
	printf("Tentativo di connessione a %s, porta %d.\n", inet_ntoa(srv_addr.sin_addr), port);
	// tentativo di connessione
	if (connect(sk, (struct sockaddr*) &srv_addr, sizeof(srv_addr))) {
		printf("Errore, connessione fallita.\n");
		exit(-1);
	} 
    printf("Connesso con successo a %s, porta %d.\n", inet_ntoa(srv_addr.sin_addr), port);

	
    return sk;
}