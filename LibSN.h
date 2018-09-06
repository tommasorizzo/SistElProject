#ifndef _LIBSN_H_
#define _LIBSN_H_


#define BUFLEN 140 // 140 caratteri massima lunghezza

#define ABORT "Errore durante l'esecuzione. Uscita.\n" // stringa di errore

// Funzione di stampa errori
void error(char *);

// Ricezione messaggio da stdin (lungh. max BUFLEN)
void get_input(char *);

// Ricezione messaggio da stdin (lungh. max BUFLEN)
// Ritorna 1 in caso di errore, altrimenti 0
int receive_msg(int, char *);

// Invia stringa (lyngh. max BUFLEN) al server
// Ritorna 1 in caso di errore, altrimenti 0
int send_msg(int, char *);

#endif 