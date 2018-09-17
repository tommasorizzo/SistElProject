#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "LibSN.h"

#define THREADS    4 
#define MAX_POSTS 10

/* 
 * Autore: Tommaso Rizzo - settembre 2018 
 */

// bacheca: struttura per gestire i post che può vedere ogni client
struct walls {
	int       		    num_post; // tiene conto del numero dei post sinora scritti
	char post[MAX_POSTS][BUFLEN]; // formato da: tm* tm_info + message
} wall[THREADS];

// struttura proprietaria di ogni client che si connette 
static struct users { 
	pthread_t  	 thread; // thread che gestisce il client a lui associato
	int     	     id; // id del thread
	sem_t 	      event; // semaforo che risveglia il thread
	int 		   busy; // variabile che segnala se il thread è occupato
	int   	  	 logged; // variabile che segnala se l'utente è connesso (dopo procedura di autenticazione)
	char  email[BUFLEN]; // memorizza indirizzo email dell'utente
	int friend[THREADS]; // 1 se si è amici del client gestito dal thread i-esimo
	int 	   	   fdes; // file descriptor del client (vale -1 se non è connesso)
} user[THREADS];

// file 'users.txt' contentente email e password cifrate degli utenti registrati
static FILE *file; 

static sem_t           freethread; // semaforo per bloccare il main se tutti i thread sono occupati 
static pthread_mutex_t      glock; // mutex per gestire le variabili globali


// dichiarazione funzioni
static int    server_init(int, char *);
static void *  handle_req(	   void *);
static void   process_req(        int);
static void      cmd_help(     char *);
static void     cmd_wrong(     char *);

int main(int argc, char *argv[]) {
	int port, sock, i, j;
	char *ip_addr;

	// variabili per il socket di connessione
	struct sockaddr_in cl_add;
    socklen_t      cl_addrlen;
    int                 cl_sk;
	
	if (argc != 3)
		error("Errore, inserire: ./sn_server <host remoto> <porta>\n");
	
	ip_addr = argv[1];
	port    = atoi(argv[2]);
	
	// inizializzazione del socket
	sock = server_init(port, ip_addr);

	pthread_mutex_init(&glock, 	NULL); // inizializzazione mutex di global_lock
	sem_init(&freethread, 0, THREADS); // inizializzazione semaforo per il main

	// inizializzazione e creazione thread
	pthread_mutex_lock(&glock);
	for(i = 0; i < THREADS; i++) {
		user[i].id     = i;
		user[i].busy   = 0;
		user[i].logged = 0;
		for (j = 0; j < THREADS; j++)
			user[i].friend[j] = 0;
		wall[i].num_post = 0;
		memset(user[i].email, 0, BUFLEN);
		sem_init(&user[i].event, 0, 0); // inizializzazione semaforo evento
 		if (pthread_create(&user[i].thread, NULL, handle_req, (void *) &user[i].id))
			error_t("errore nella creazione\n", i);
 		printf("THREAD %d: pronto\n", i);
	}
	pthread_mutex_unlock(&glock);
	
	while(1) {
	// Accetta la connessione
        cl_addrlen = sizeof(cl_add);
		memset(&cl_add, 0, cl_addrlen);
        cl_sk = 	accept(sock, (struct sockaddr*)&cl_add, &cl_addrlen);
        if (cl_sk < 0) error("MAIN: non è stato possibile accettare il client.\n");
		printf ("MAIN: connessione stabilita con il client %s:%d\n", inet_ntoa(cl_add.sin_addr), port);
		
		printf("MAIN: in attesa di un thread libero\n");
		sem_wait(&freethread); // in attesa di un thread libero
		
		pthread_mutex_lock(&glock);
		for(i = 0; i < THREADS; i++)   
			if (!user[i].busy) // thread libero trovato, è il thread i-esimo
				break;
		if (i==THREADS) error("MAIN: errore nella gestione dei thread.\n");
		user[i].busy = 1;      // thread diventa occupato
		user[i].fdes = cl_sk;  // passaggio del client file descriptor
		pthread_mutex_unlock(&glock);
		
		printf("MAIN: è stato selezionato il thread %d\n", i);
		sem_post(&user[i].event); // risveglia il thread libero

	}

	close(sock); // chiusura server
	for (i = 0; i < THREADS; i++) {
		void *status;
        pthread_join(user[i].thread, (void **) &status);
	}
	free(ip_addr);
}

static int server_init(int myport, char *ip_addr)
{
    struct sockaddr_in my_addr;
    int sk;

    memset(&my_addr, 0, sizeof(my_addr));            
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(myport);
    // inet_pton(AF_INET, ip_addr, &my_addr.sin_addr);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	
    sk = socket(AF_INET, SOCK_STREAM, 0);
	if (sk < 0)
		error("Errore nell'apertura del socket.\n");
	
    if (bind(sk, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0)
        error("Errore nella procedura di binding.\n");

    if (listen(sk, 10) < 0)
        error("Errore nella procedura di listening.\n");

    printf ("Indirizzo: %s (Porta: %d)\n",inet_ntoa(my_addr.sin_addr), myport);
	
    return sk;
}
	
static void * handle_req(void *tid) {
	int id = *((int *) tid);
	int i, cl_des;
	
	// get socket
	pthread_mutex_lock(&glock);
	cl_des = user[id].fdes; // riceve il file descriptor del client
	pthread_mutex_unlock(&glock);

	while(1) {
		sem_wait(&user[id].event); // aspetta di essere risvegliato dal main
		
	// gestione della richiesta del client
		process_req(id);
		
	// quit ricevuto
		close(cl_des); // chiude il socket
		pthread_mutex_lock(&glock);
		user[id].busy     =  0;
		user[id].logged   =  0;
		user[id].fdes     = -1; 
		wall[id].num_post =  0;
		for (i = 0; i < THREADS; i++) {
			user[id].friend[i] = 0; // rimuovi amici al logout
			user[i].friend[id] = 0; // mutualmente
		}
		pthread_mutex_unlock(&glock);
		
		printf("THREAD %d: task eseguito, pronto per servire una nuova richiesta\n", id);
		sem_post(&freethread); // segnala al main che il thread è di nuovo libero		
	}
	
    pthread_exit(NULL);	
}

static void process_req(int id) {
	int cl_des, i, r, dim;

	time_t rawtime;     // variabili per estrarre
	struct tm *tm_info; // data e orario correnti

	char action[BUFLEN];      // buffer dove viene memorizzata la richiesta del client
	char utenti[BUFLEN];      // buffer che scansiona il file 'users.txt'
	char tosend[BUFLEN];	  // buffer per la risposta del server
	char tmp[BUFLEN]; // buffer di appoggio
	
	memset(utenti, 0, BUFLEN);
	memset(action, 0, BUFLEN);

// get socket
	pthread_mutex_lock(&glock);
	cl_des = user[id].fdes; // riceve il file descriptor del client
	pthread_mutex_unlock(&glock);
	
	while (1) {
	// ricevi messaggio dal client
		r = receive_dim(cl_des, &dim);
		if (r) error(ABORT);
		r = receive_msg(cl_des, action, dim);
		if (r) error(ABORT);

	// controlla il comando ricevuto e i suoi argomenti
		char *arg[3];     // buffer di controllo degli argomenti immessi  

		strcpy(tmp, action);
		arg[0] = strtok( tmp, " \n"); // comando
		arg[1] = strtok(NULL, " \n"); // (opzionale) secondo comando - email
		arg[2] = strtok(NULL, " \n"); // (opzionale) terzo comando - password
		
		printf("THREAD %d: ricezione comando %s\n", id, arg[0]);
		memset(tosend, 0, BUFLEN);
		
	// caso: register 
		if (!strcmp(arg[0], "register")) {
			// printf("DEBUG: ricevuto 'register'\n");
			if(user[id].logged)
				strcpy(tosend, "L'utente ha già effettuato il login\n");
			else {	
			// controllo indirizzo email
				pthread_mutex_lock(&glock);
			// apertura file in reading e append
				file = fopen("users.txt", "a+"); 
				if (file == NULL) error_t("errore nell'apertura del file\n", id);
			// scansiona ogni parola per vedere se già presente
				while (fgets(utenti, BUFLEN, file) != NULL) {
					if (strstr(utenti, arg[1]) != NULL) { // già presente
						strcpy(tosend, "Registrazione fallita, utente già presente.\n"); // per il client
						break;
					}
				}
				if (strstr(utenti, arg[1]) == NULL) {
				// procede alla registrazione
					fprintf(file, "%s %s\n", arg[1], arg[2]);
					strcpy(tosend, "Registrazione eseguita.\n"); // messaggio per il client
				}
				fclose(file);
				pthread_mutex_unlock(&glock);
			}
		}

	// caso: login
		else if (!strcmp(arg[0], "login")) {
			char email[BUFLEN]; // email dell'utente (dal file)
			char pswrd[BUFLEN]; // password dell'utente (dal file)
			char ipass[BUFLEN]; // password cifrata (da socket)
			char       salt[2]; // salt (dal file)
			
			int already_log = 0; // login già eseguito da un altro client
			for (i=0; i < THREADS; i++)
				if(user[i].logged && !strcmp(user[i].email, arg[1])) {
					already_log = 1;
					break;
				}
			if(user[id].logged || already_log)
				strcpy(tosend, "l'utente ha già effettuato il login.\n");
			else {	
				pthread_mutex_lock(&glock);
			// apertura file in read mode
				file = fopen("users.txt", "r"); 
				if (file == NULL) error_t("errore nell'apertura del file\n", id);
			// controllo email
				while (fgets(utenti, BUFLEN, file)) {
				// estraggo email e password dalla riga corrente
					sscanf(utenti, "%s %s", email, pswrd);
					if (!strcmp(arg[1], email)) // trovato
						break;
				}
				if (strcmp(arg[1], email) && fgets(utenti, BUFLEN, file) == NULL)
					strcpy(tosend, "Login fallito, utente non registrato.\n"); // per il client
			// controllo password
				else {
				// estraggo il salt
					salt[0] = pswrd[0];
					salt[1] = pswrd[1];
					strcpy(ipass, crypt(arg[2], salt)); // cifro password ricevuta
					if (strcmp(ipass, pswrd))
						strcpy(tosend, "Password non corretta!\n"); // per il client
				}
				if (!strcmp(arg[1], email) && !strcmp(ipass, pswrd)){
				// effettua login
					user[id].logged = 1;           // utente connesso
					strcpy(user[id].email, email); // memorizza email
					strcpy(tosend, "Benvenuto!\n"); // messaggio per il client
				}				
				fclose(file);
				pthread_mutex_unlock(&glock);
			}
		}

	// caso: friend
		else if(!strcmp(arg[0], "friend") && arg[1] != NULL && arg[2] == NULL) {
			if (!user[id].logged)
				strcpy(tosend, "Attenzione, eseguire prima il login.\n");
			else {
				for(i = 0; i < THREADS; i++)
					if(!strcmp(arg[1], user[i].email)) // trovato amico
						break;						   // è l'user[i]
				if (i == id)
					strcpy(tosend, "Non puoi diventare amico con te stesso.\n");
				else if (i == THREADS)
					strcpy(tosend, "Utente non collegato.\n");
				else {
					if (!user[i].logged) // mi sa che non server
						strcpy(tosend, "Utente non collegato.\n");
					else {
						if (user[id].friend[i]) {
							strcpy(tosend, "Sei già amico con ");
							strcat(tosend, arg[1]);
							strcat(tosend, "\n");
						} else {
							pthread_mutex_lock(&glock);
							user[id].friend[i] = 1;
							user[i].friend[id] = 1;
							pthread_mutex_unlock(&glock);
							strcpy(tosend, "Adesso ");
							strcat(tosend, arg[1]);
							strcat(tosend, " è tuo amico.\n");
						}
					}
				}
			}
		}
		
	// caso: post 
		else if (!strcmp(arg[0], "post")) {
			pthread_mutex_lock(&glock);
			if (!user[id].logged)
				strcpy(tosend, "Attenzione, eseguire prima il login.\n");
			else {
				char   msg[BUFLEN]; // d'appoggio per memorizzare il post
				char fwall[BUFLEN]; // bacheca piena
				int full = 0; // numero di amici con la bacheca piena
				memset(	 msg, 0, BUFLEN);
				memset(fwall, '\0', BUFLEN);
				time(&rawtime);
				tm_info = localtime(&rawtime);						
			// composizione del post dentro il buffer msg
				strftime(msg,BUFLEN, "%d/%m/%Y, %H:%M, ", tm_info);
				strcat(msg, user[id].email);
				strcat(msg, ": \"");	
				strcat(msg, action+sizeof("post"));
				strcat(msg, "\"\n");
				for (i = 0; i < THREADS; i++) // manda il post agli amici
					if (user[id].friend[i]) {
						int np = wall[i].num_post; // posizione del nuovo post nella bacheca dell'amico
						if (np == MAX_POSTS) {
							full++;
							strcat(fwall, user[i].email);
						} else {
						strcpy(wall[i].post[np], msg);
						wall[i].num_post++;
						// printf("DEBUG: %s's wall: %s", user[i].email, wall[i].post[np]);
						}
					}
				strcpy(tosend, "Post inviato agli amici\n");
				if (full) {
					sprintf(tosend + strlen(tosend), "%d amico/i con bacheca piena: %s\n", full, fwall);
				}
			}
			pthread_mutex_unlock(&glock);
		}
		
	// caso: view 
		else if (!strcmp(arg[0], "view") && arg[1] == NULL) {
			pthread_mutex_lock(&glock);
			if (!user[id].logged)
				strcpy(tosend, "Attenzione, eseguire prima il login.\n");
			else {
				int np = wall[id].num_post;
				// printf("DEBUG: numero di post da mostrare: %d\n", np);
				if (!np)
					strcpy(tosend, "Nessun post da mostrare.\n");
				else {
					char allposts[MAX_POSTS*BUFLEN]; // stringa contente tutti i post
					memset(allposts, 0, sizeof(allposts));
					for (i = 0; i < np; i++) {
						strcat(allposts, wall[id].post[i]); // concatena ogni post in allposts
						// printf("DEBUG: %d: %s", i, allposts);
					}
					wall[id].num_post = 0;
					if (strlen(allposts) > BUFLEN) { // controllo dimensioni buffer
						dim = (int) strlen(allposts);
						r = send_dim(cl_des, dim);
						if (r) error(ABORT);
						r = send_msg(cl_des, allposts);
						if (r) error(ABORT);
						continue;
					} else
						strcpy(tosend, allposts); // si può inviare 'tosend' regolarmente
				}
			}
			pthread_mutex_unlock(&glock);
		}
		
	// caso: help 
		else if(!strcmp(arg[0], "help") && arg[1] == NULL) 
			cmd_help(tosend);
		
	// caso: quit
		else if (!strcmp(arg[0], "quit") && arg[1] == NULL)
			strcpy(tosend, "server_quit");
	
	// caso: comando errato
		else
			cmd_wrong(tosend);
	
	// invia messaggio al client -- caso 'view' trattato a parte
		dim = (int) strlen(tosend);
		r = send_dim(cl_des, dim);
		if (r) error(ABORT);
		r = send_msg(cl_des, tosend);
		if (r) error(ABORT);
		
		if (!strcmp(arg[0], "quit")) {
			printf("THREAD %d: chiusura connessione\n", id);
			return;
		}

	}
}

void cmd_help(char *msg){
    char helps[] = " Comandi disponibili:\n"
        " help --> mostra l'elenco dei comandi disponibili\n"
        " register --> registrazione al sistema\n"
        " login --> autenticazione\n"
        " friend <nuovoamico> --> aggiunge il nuovo amico specifica come argomento\n"
        " post <testo del messaggio> --> nuovo post\n"
        " view --> visualizza gli ultimi post fatti dagli amici\n"
        " quit --> esce dal sistema\n";
    strcpy(msg, helps);
}

void cmd_wrong(char *msg){
    char wrongs[] = " Comando non disponibile. Ecco l'elenco dei commandi disponibili:\n"
        " help --> mostra l'elenco dei comandi disponibili\n"
        " register --> registrazione al sistema\n"
        " login --> autenticazione\n"
        " friend <nuovoamico> --> aggiunge il nuovo amico specifica come argomento\n"
        " post <testo del messaggio> --> nuovo post\n"
        " view --> visualizza gli ultimi post fatti dagli amici\n"
        " quit --> esce dal sistema\n";
    strcpy(msg, wrongs);
}
