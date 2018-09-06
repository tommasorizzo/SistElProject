#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "LibSN.h"

/* 
 * Autore: Tommaso Rizzo - settembre 2018 
 */
 
// dichiarazione funzioni
int client_connect(int port, char *ip_addr);
 
int main (int argc, char *argv[]) {
	
	int sock, port, r;
	int new_cmd = 1;     // segnala se il client può inviare un nuovo comando 
	char action[BUFLEN]; // buffer contenente il comando da eseguire
	char tmp[BUFLEN];	 // buffer di appoggio
	char *arg[2];        // buffer di controllo degli argomenti immessi  
	char *ip_addr;
	
	if (argc != 3)
		error("Errore, inserire: ./sn_client <host remoto> <porta>\n");
	
	ip_addr = argv[1];
	port    = atoi(argv[2]);
	
	// inizializzazione e connessione al server
	sock = client_connect(port, ip_addr);
	
	// corpo
	while(1) {
		
	// ricevi comando da stdin se new_cmd è vero (ossia è stato risolto il precedente comando)
		if (new_cmd) { 
			printf("> ");		 
			get_input(action);   
		// controlla il comando ricevuto e i suoi argomenti
			strcpy(tmp, action);         
			arg[0] = strtok( tmp, " \n"); // comando vero e proprio
			arg[1] = strtok(NULL, " \n"); // secondo comando (opzionale)
		}	
		
	// errori di sintassi 
		if (((!strcmp(arg[0], "friend") || !strcmp(arg[0], "post")) && arg[1] == NULL) || 		 // casi con più di un parametro
			(( strcmp(arg[0], "friend") &&  strcmp(arg[0], "post")) && arg[1] != NULL)) {        // casi con un solo parametro
			printf("Errore, sintassi non corretta. Digita 'help' per maggiori informazioni.\n");
			new_cmd = 1;
			continue;
		}			
		
	// casi: email e password
		if (!strcmp(arg[0], "email") && arg[1] == NULL && !new_cmd) { // email
			get_input(action);
			// controllo comandi
			strcpy(tmp, action);
			arg[0] = strtok( tmp, " "); 
			arg[1] = strtok(NULL, " "); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Indirizzo mail non valido.\n");
				new_cmd = 1;
				continue;
			}
		} else if (!strcmp(arg[0], "password") && arg[1] == NULL && !new_cmd) { // password
			get_input(action);
			// controllo comandi
			strcpy(tmp, action);
			arg[0] = strtok( tmp, " "); 
			arg[1] = strtok(NULL, " "); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Password non valida.\n");
				new_cmd = 1;
				continue;
			}
			strcpy(action, crypt(arg[0], "b5"));
		}
	
	// invia comando al server
		r = send_msg(sock, action);
		if (r) error(ABORT);
		r = receive_msg(sock, action);
		if (r) error(ABORT);
	
	// quit
		if (!strcmp(action, "server_quit")) { 
			printf("Sei uscito dal sistema.\n");
			break;
		}
	// register email
		else if (!strcmp(action, "reg_email")) { 
			printf("Inserisci il tuo indirizzo di posta elettronica:\n");
			strcpy(arg[0], "email");
			new_cmd = 0;
		} 
	// register password
		else if (!strcmp(action, "reg_pw")) {    
			printf("Scegli la tua password:\n");
			strcpy(arg[0], "password");
			new_cmd = 0;
		} 
	// login email	
		else if (!strcmp(action, "log_email")) {  
			printf("Inserisci il tuo nome utente (indirizzo di posta elettronica):\n");
			strcpy(arg[0], "email");
			new_cmd = 0;
		} 
	// login password	
		else if (!strcmp(action, "log_pw")) {    
			printf("Inserisci la password:\n");
			strcpy(arg[0], "password");
			new_cmd = 0;
		} 
	// tutti gli altri casi
		else {                  
			printf("%s", action); // stampa il messaggio ricevuto 
			new_cmd = 1;		  // e prepara a ricevere il nuovo comando 	
		}
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


/* 
---------------------------------------------------OLD VERSION---------------------------------------------------
-----------------------------------------------------------------------------------------------------------------
	// casi con più di un parametro: friend e post 
		if ((!strcmp(arg[0], "friend") || !strcmp(arg[0], "post")) && arg[1] != NULL) {
			strcpy(tmp, action); // in action è conservato tutto il comando
			empty(action);
		} else if ((!strcmp(arg[0], "friend") || !strcmp(arg[0], "post")) && arg[1] == NULL) {
			printf("Errore, sintassi non corretta. Digita 'help' per maggiori informazioni.\n");
			empty(action);
			continue;
		}		
	// in tutti gli altri casi serve un parametro solo 
		else if (arg[1]!= NULL) { 
			printf("Errore, inserito più di un parametro. Digita 'help' per maggiori informazioni.\n");
			empty(action);
			continue; 
		} 
		
	// casi con un solo parametro: register, login, view, help, quit, (email, password)
		if ((!strcmp(arg[0], "register") || !strcmp(arg[0], "login")) && arg[1] == NULL) { // register, login
			strcpy(  temp,  arg[0]);
			strcpy(arg[0], "email");
		} else if (!strcmp(arg[0], "email") && arg[1] == NULL) { // email
			get_input(tmp);
			// controllo comandi
			arg[0] = strtok( tmp, " "); 
			arg[1] = strtok(NULL, " "); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Indirizzo mail non valido.\n");
				empty(action);
				continue;
			}
			strcpy( temp,      arg[0]);
			strcpy(arg[0], "password");
		} else if (!strcmp(arg[0], "password") && arg[1] == NULL) { // password
			get_input(tmp);
			// controllo comandi
			arg[0] = strtok( tmp, " "); 
			arg[1] = strtok(NULL, " "); 
			if (arg[1] != NULL) {
				printf("Errore, inserito più di un parametro. Password non valida.\n");
				empty(action);
				continue;
			}
			strcpy(temp, crypt(arg[0], "b5"));
			empty(action);
		} else if ((!strcmp(arg[0], "view") || !strcmp(arg[0], "help")) && arg[1] == NULL) { // view, help 
			strcpy(temp, arg[0]);
			empty(action);
		} else {
			printf("Errore, sintassi non corretta. Digita 'help' per maggiori informazioni.\n");
			empty(action);
			continue;
		}
 */