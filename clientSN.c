#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "LibSN.h"

/* 
 * Autore: Tommaso Rizzo - settembre 2018 
 */
 
// dichiarazione funzioni
int  client_connect(int, 			char *);
void 	create_salt(                char[]);
 
int main (int argc, char *argv[]) {
	
	int sock, port, r, dim;
	
	char action[BUFLEN]; // buffer contenente il comando da eseguire e la risposta del server
	char 	tmp[BUFLEN]; // buffer di appoggio
	char 		salt[2]; // salt per cifrare la password
	char 	   *ip_addr;
	
	if (argc != 3)
		error("Errore, inserire: ./sn_client <host remoto> <porta>\n");
	
	ip_addr = argv[1];
	port    = atoi(argv[2]);
	
	// inizializzazione e connessione al server
	sock = client_connect(port, ip_addr);
	
	// corpo
	while(1) {
	
		printf("> ");		 
    // ricevi l'input da stdin
		do{
		  get_input(action);
		  strtok(action, "\n");
		} while(!strcmp(action, "\n")); // così tolgo il "\n"
		
	// controlla il comando ricevuto e i suoi argomenti
		char *arg[2]; // buffer di controllo degli argomenti immessi  
		strcpy(tmp, action);         
		arg[0] = strtok( tmp, " \n"); 
		arg[1] = strtok(NULL, " \n"); 
		
	// caso: register 
		if (!strcmp(arg[0], "register") && arg[1] == NULL) {
			char credenziali[BUFLEN];
			printf("Inserisci il tuo indirizzo di posta elettronica:\n");
			do { // registra indirizzo email
				get_input(credenziali);
				strtok(credenziali, "\n");
			} while(!strcmp(credenziali, "\n")); 
			// controlla il comando ricevuto e i suoi argomenti
			sprintf(tmp, "%s", credenziali);         
			arg[0] = strtok( tmp, " \n"); 
			arg[1] = strtok(NULL, " \n"); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Indirizzo mail non valido.\n");
				continue;
			}
			strcat(action,    " ");	// concatena email al 
			strcat(action, arg[0]); // comando 'register'
			// printf("DEBUG: action=%s\n", action);
			printf("Scegli la tua password:\n");
			do { // registra password	
				get_input(credenziali);
				strtok(credenziali, "\n");
			} while(!strcmp(credenziali, "\n")); 
			// controlla il comando ricevuto e i suoi argomenti
			sprintf(tmp, "%s", credenziali);         
			arg[0] = strtok( tmp, " \n"); 
			arg[1] = strtok(NULL, " \n"); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Password non valida.\n");
				continue;
			}
			strcat(action, " ");
			create_salt(salt);
			char crypted[BUFLEN];
			sprintf(crypted, "%s", crypt(arg[0], salt))	;
			strcat(action, crypted);
		} else if (!strcmp(arg[0], "register") && arg[1] != NULL) {
			printf("Errore, inserito più di un parametro. Indirizzo mail non valido.\n");
			continue;			
		}
		
	// caso: login 
		if (!strcmp(arg[0], "login") && arg[1] == NULL) {
			char credenziali[BUFLEN];
			printf("Inserisci il tuo nome utente (indirizzo di posta elettronica):\n");
			get_input(credenziali); // registra indirizzo email
			// controlla il comando ricevuto e i suoi argomenti
			sprintf(tmp, "%s", credenziali);         
			arg[0] = strtok( tmp, " \n"); 
			arg[1] = strtok(NULL, " \n"); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Indirizzo mail non valido.\n");
				continue;
			}
			strcat(action,    " ");	 
			strcat(action, arg[0]); 
			printf("Inserisci la password:\n");
			get_input(credenziali); // registra password
			// controlla il comando ricevuto e i suoi argomenti
			sprintf(tmp, "%s", credenziali);         
			arg[0] = strtok( tmp, " \n"); 
			arg[1] = strtok(NULL, " \n"); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Password non valida.\n");
				continue;
			}
			strcat(action, " ");
			strcat(action, arg[0]);
		} else if (!strcmp(arg[0], "login") && arg[1] != NULL) {
			printf("Errore, inserito più di un parametro. Indirizzo mail non valido.\n");
			continue;			
		}
		
	// invia comando al server
		dim = (int) strlen(action);
		// printf("DEBUG: Invio la stringa '%s' (dim. %d)\n", action, dim);
		r = send_dim(sock, dim);
		if (r) error(ABORT);
		r = send_msg(sock, action);
		if (r) error(ABORT);
		
	// ricevi risposta del server
		r = receive_dim(sock, &dim);
		if (r) error(ABORT);
		r = receive_msg(sock, action, dim);
		if (r) error(ABORT);

	// quit
		if (!strcmp(action, "server_quit")) { 
			printf("Sei uscito dal sistema.\n");
			break;
		} else                  
			printf("%s", action); // stampa il messaggio ricevuto 
	}
	
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

void create_salt(char salt[]) {
	int r;
// Inizializzo il generatore di numeri casuali
	srand(time(NULL));
// Genero i caratteri del salt in modo tale che siano stampabili
	r = rand();
	r = r % ('z' - 'a');
	salt[0] = 'a' + r;
	r = rand();
	r = r % ('z' - 'a');
	salt[1] = 'a' + r;
	
//	printf("Salt: %c%c\n", salt[0], salt[1]);
}
