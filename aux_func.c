#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

void getError(char errmsg[LEN], char type[10], int error) {
	//generare mesaj eroare pentru client
	if (strcmp(type, "client") == 0) {
		switch(error) {
			case -1:
				sprintf(errmsg, "%d : Clientul nu este autentificat\n", error);
				break;
			case -2:
				sprintf(errmsg, "%d : Sesiune deja deschisa\n", error);
				break;
		}
	}
	//generare mesaj eroare pentru ATM
	if (strcmp(type, "atm") == 0) {
		switch(error) {
			case -2:
				sprintf(errmsg, "ATM> %d : Sesiune deja deschisa\n", error);
				break;
			case -3:
				sprintf(errmsg, "ATM> %d : Pin gresit\n", error);
				break;
			case -4:
				sprintf(errmsg, "ATM> %d : Numar card inexistent\n", error);
				break;	
			case -5:
				sprintf(errmsg, "ATM> %d : Card blocat\n", error);
				break;	
			case -9:
				sprintf(errmsg, "ATM> %d : Suma nu este multiplu de 10\n", error);
				break;
			case -8:
				sprintf(errmsg, "ATM> %d : Fonduri insuficiente\n", error);
				break;	
		}
	}
	//generare mesaj eroare pentru serviciul unlock
	if (strcmp(type, "unlock") == 0) {
		switch(error) {
			case 1:
				sprintf(errmsg, "UNLOCK> Trimite parola secreta\n");
				break;
			case 2:
				sprintf(errmsg, "UNLOCK> Client deblocat\n");
				break;	
			case -4:
				sprintf(errmsg, "UNLOCK> %d : Numar card inexistent\n", error);	
				break;
			case -6:
				sprintf(errmsg, "UNLOCK> %d : Operatie esuata\n", error);
				break;
			case -7:
				sprintf(errmsg, "UNLOCK> %d : Deblocare esuata\n", error);
				break;	

		}

	}
}

void readData(int fd, Client *clients) {
	int nr = 0;
	int count = 0;
	int token = 0;
	int i = 0;
	Client client;
	char c[100];
	while ((nr = read(fd, &c[count++], 1)) > 0) {
		if (c[count - 1] == ' ') {
			c[count - 1] = '\0';
			//citesc primul cuvant
			if (token == 0) {
				sprintf(client.last_name, "%s", c);
				count = 0;
				token++;
				continue;
			}
			//citesc al doilea cuvant
			if (token == 1) {
				sprintf(client.first_name, "%s", c);
				count = 0;
				token++;
				continue;
			}
			//citesc al treilea cuvant
			if (token == 2) {
				client.card_number = atoi(c);
				count = 0;
				token++;
				continue;
			}
			//citesc al patrulea cuvant
			if (token == 3) {
				client.pin = atoi(c);
				count = 0;
				token++;
				continue;
			}
			//citesc al cincilea cuvant
			if (token == 4) {
				sprintf(client.password, "%s", c);
				count = 0;
				token++;
				continue;
			}
			count = 0;
		
		}
		//s-a terminat randul
	 	if (c[count - 1] == '\n') {
			c[count - 1] = '\0';
			client.sold = atof(c);
			client.blocked = 1;
			client.logged = 1;
			count = 0;
			token = 0;
			clients[i] = client;
			i++;
		}
	}	
}

int getNrClients(int fd) {
	int nr_clients = 0;
	char c[10];
	read(fd, c, 2);
	c[strlen(c)] = '\0';
	nr_clients = atoi(c);

	return nr_clients;
}

char *getCommand(char *buffer) {
	char *dup = strdup(buffer);
    char *tok = strtok(dup, " ");
    return tok;
}

int verifyLogin(char *command, Client *clients, int nr_clients) {
	char *dup = strdup(command);
	char *tok = strtok(dup, " ");
	int card_number;
	int pin;
	int i = 0;
	int err = -4;
	while(tok != NULL) {
		if (i == 1) {
			card_number = atoi(tok);
		}
		if (i == 2) {
			pin = atoi(tok);
		}
		i++;
		tok = strtok(NULL, " ");
	}

	for (i = 0; i < nr_clients; i++) {
		Client client = clients[i];
		if (client.card_number == card_number) {
			//clientul este blocat
			if (client.blocked == 0) {
				return -5;
			}
			//clientul este logat
			if (client.logged == 0) {
				return -2;
			}
			//pinul este valid
			if (client.pin == pin) {
				err = 1;
			}
			else {
				err = -3;
			}
		}
	}
	return err;
}
int clientIsLogged(char *buffer) {
	char *dup = strdup(buffer);
	char *tok = strtok(dup, " ");
	int i = 0;
	while(tok != NULL) {
		if (i == 1) {
			if (strcmp(tok, "Welcome") == 0) {
				return 0;
			}
			else {
				return 1;
			}
		}
		i++;
		tok = strtok(NULL, " ");
	}
} 

int getClient(Client *clients, int card_number, int nr_clients) {
	int i = 0;

	for (i = 0; i < nr_clients; i++) {
		Client client = clients[i];
		if (client.card_number == card_number) {
			return i;
		}
	}		
}

double getValue(char *command) {
	char *dup = strdup(command);
	char *tok = strtok(dup, " ");
	double value;
	int i = 0;
	while(tok != NULL) {
		if (i == 1) {
			value = atof(tok);
		}
		i++;
		tok = strtok(NULL, " ");
	}

	return value;

}

int getCardNumber(char *command) {
	char *dup = strdup(command);
	char *tok = strtok(dup, " ");
	int card_number;
	int i = 0;
	while(tok != NULL) {
		if (i == 1) {
			card_number = atoi(tok);
		}
		i++;
		tok = strtok(NULL, " ");
	}

	return card_number;

}

int verifyUnlock(int card_number, Client *clients, int nr_clients) {
	int err = -4;
	int i;
	for (i = 0; i < nr_clients; i++) {
		Client client = clients[i];
		if (client.card_number == card_number) {
			if (client.blocked == 0) {
				err = 1;
			}
			else {
				err = -6;
			}
		}
	}
	return err;	
}

char *getPassword(char *command) {
	char *dup = strdup(command);
	char *tok = strtok(dup, " ");
	int i = 0;
	while(tok != NULL) {
		if (i == 1) {
			break;
		}
		i++;
		tok = strtok(NULL, " ");
	}

	return tok;	
}

int verifyPassword(char *command, Client *clients, int nr_clients) {
	int card_number;
	char *dup = strdup(command);
	char *tok = strtok(dup, " ");
	card_number = atoi(tok);
	char *password = getPassword(command);
	int i;
	int err = -7;
	for (i = 0; i < nr_clients; i++) {
		Client client = clients[i];
		if (client.card_number == card_number) {
			if (strcmp(client.password, password) == 0) {
				err = 2;
			}
		}
	}
	return err;
}