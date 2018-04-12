#define LEN 256

typedef struct {
	char last_name[13];
	char first_name[13];
	int card_number;
	int pin;
	char password[17];
	double sold;
	int blocked; //0 if the client is blocked, 1 otherwise
	int logged; //0 if the client is logged , 1 otherwise
} Client;

//copiaza in errmsg un mesaj corespunzator numarului erorii
void getError(char *errmsg, char type[10], int error);
//citeste datele despre clienti din fisier si populeaza structura
void readData(int fd, Client *clients);
//returneaza numarul de clienti(de pe prima linie din fisier)
int getNrClients(int fd);
//returneaza comanda(primul termen) din stringul de input(citit de la tastatura)
char *getCommand(char *buffer);
//verifica daca comanda login se realizeaza cu succes si intoarce o eroare altfel
int verifyLogin(char *command, Client *clients, int nr_clients);
//returneaza indicele clientului cu card_number
int getClient(Client *clients, int card_number, int nr_clients);
//returneaza valoarea(suma) pentru comenzile getmoney si putmoney
double getValue(char *command);
//returneaza numarul cardului in cazul comenzii login
int getCardNumber(char *command);
//verifica daca unlock se realizeaza cu succes si intoarce o eroare
int verifyUnlock(int card_number, Client *clients, int nr_clients);
//returneaza parola dintr-o comanda de tipul (<card_number> <password>)
char *getPassword(char *command);
//verifica daca parola este corecta si returneaza o eroare
int verifyPassword(char *command, Client *clients, int nr_clients);