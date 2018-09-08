#ifndef _LIBSN_H_
#define _LIBSN_H_


#define BUFLEN 512 // 140 caratteri massima lunghezza

#define ABORT "Errore durante l'esecuzione. Uscita.\n" // stringa di errore

// Funzione di stampa errori
void error(char *);

// Funzione di stampa errori del thread specificato
void error_t(char *, int);

// Ricezione messaggio da stdin (lungh. max BUFLEN)
void get_input(char *);

// Ricezione dimensione e messaggio da server
// Ritorna 1 in caso di errore, altrimenti 0
int receive_dim(int,       int *);
int receive_msg(int, char *, int);

// Invia dimensione e stringa al server
// Ritorna 1 in caso di errore, altrimenti 0
int send_dim(int,    int);
int send_msg(int, char *);

#endif 