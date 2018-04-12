#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "lib.h"

void error(char *msg)
{
    printf("%s\n",msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, clilen, sockudp;
    char buffer[LEN];
    struct sockaddr_in serv_addr, cli_addr, my_sockaddr;
    int n, i, j, count = 0;

    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds

    int fd = open(argv[2], O_RDONLY, 0666);
    Client client;
    int nr_clients = getNrClients(fd);
    Client clients[nr_clients];
    readData(fd, clients);

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    sockudp = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockudp < 0) 
    	error("-10 : Eroare de apel socket\n");

    //adaugam noul descriptor(socketul udp)
    FD_SET(sockudp, &read_fds);

    //initializare structura udp
    memset((char *) &my_sockaddr, 0, sizeof(struct sockaddr_in));
    my_sockaddr.sin_family = AF_INET;
    my_sockaddr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    my_sockaddr.sin_port = htons(atoi(argv[1]));

    if (bind(sockudp, (struct sockaddr *) &my_sockaddr, sizeof(my_sockaddr)) < 0) 
        error("ATM> -10 : Eroare de apel bind\n");
     
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    	error("ATM> -10 : Eroare de apel socket\n");
     
    portno = atoi(argv[1]);

    //initializare structura tcp
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(portno);
     
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
        error("ATM> -10 : Eroare de apel bind\n");
     
    listen(sockfd, nr_clients + 1);

    //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds 
    FD_SET(sockfd, &read_fds);

    //adaugam descriptorul pentru citire de la tastatura
    FD_SET(0, &read_fds);
     
    if (sockfd > sockudp) {
        fdmax = sockfd;
    }
    else {
    	fdmax = sockudp;
    }

    int quit = 0;
    int secret_password = 0;
    Client loggedClient;
    Client currentClient;
    int sock_map_logged[nr_clients + 1];
    int sock_map_lastlog[nr_clients + 1];
    memset(sock_map_logged, 0, nr_clients + 1);
    memset(sock_map_logged, 0, nr_clients + 1);
    // main loop
	while (1) {
		if (quit == 1) {
			break;
		}

		char client[100];
		tmp_fds = read_fds;

		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ATM> -10 : Eroare de apel select\n");
	
		for(i = 0; i <= fdmax; i++) {
			//verifica daca descriptorul e in multime
			if (FD_ISSET(i, &tmp_fds)) {
				//citesc de la tastatura
				if (i == 0) {
					char command[LEN];
                    memset(command, 0 , LEN);
                    fgets(command, LEN-1, stdin);
                    command[strlen(command) - 1] = '\0';
                    
                    if (strcmp(command, "quit") == 0) {
                    	quit = 1;
                    	int j;
                    	for (j = sockfd + 1; j <= fdmax; j++) {
                    		sprintf(buffer, "Serverul se va deconecta!\n");
                    		int s = send(j, buffer, strlen(buffer), 0);
							//inchid socketii si ii scot din multime
							close(j); 
							FD_CLR(j, &read_fds);                    	
                    	}
                    	//inchid socketul de udp si il scot din multime
                    	close(sockudp);
                    	FD_CLR(sockudp, &read_fds);
                    }
                    break;
				}
				else if (i == sockudp) {
					//primesc ceva pe socketul udp
					memset(buffer, 0, LEN);
					socklen_t len = sizeof(my_sockaddr);
					if ((n = recvfrom(i, buffer, LEN, 0, (struct sockaddr*)&my_sockaddr, &len)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("server: socket %d hung up\n", i);
						} else {
							error("ATM> -10 : Eroare de apel recvfrom\n");
						}
						//inchid socketul si il scot din multime
						close(i); 
						FD_CLR(i, &read_fds);
					}
					else {
						char *command = getCommand(buffer);
						//primesc comanda unlock
						if (strcmp(command, "unlock") == 0) {
							int card_number = getCardNumber(buffer);
							int err = verifyUnlock(card_number, clients, nr_clients);
							if (err == 1) {
								secret_password = 1;
							}
							getError(buffer, "unlock", err);
							int s = sendto(sockudp, buffer, strlen(buffer), 0, (struct sockaddr*)&my_sockaddr, sizeof(my_sockaddr));
							if (s < 0) {
                            error("ERROR writing to socket");
                        	}
                        	continue;

						}
						//astept parola secreta
						if(secret_password == 1) {
							int err = verifyPassword(buffer, clients, nr_clients);
							char *dup = strdup(buffer);
							char *tok = strtok(dup, " ");
							int card_number = atoi(tok);
							getError(buffer, "unlock", err);
							if (err == 2) {
								int index = getClient(clients, card_number, nr_clients);
								clients[index].blocked = 1;
								secret_password = 0;
							}

							int s = sendto(sockudp, buffer, strlen(buffer), 0, (struct sockaddr*)&my_sockaddr, sizeof(my_sockaddr));
							if (s < 0) {
                            error("ERROR writing to socket");
                        	}
						}

					} 

				}

				else if (i == sockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ATM> -10 : Eroare de apel accept\n");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);
				}
					
				else {
					// am primit date pe unul din socketii cu care vorbesc cu clientii(tcp)
					//actiunea serverului: recv()
					memset(buffer, 0, LEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("server: socket %d hung up\n", i);
						} else {
							error("ATM> -10 : Eroare de apel recv\n");
						}
						//inchid socketul si il scot din multime
						close(i); 
						FD_CLR(i, &read_fds);
					} 
					
					else { //recv intoarce >0
						int j;
						for (j = 0; j <= fdmax; j++) {
							if (j >= sockfd && i==j) {
								char *command = getCommand(buffer);
								if (strcmp(command, "login") == 0) {
									int index = getClient(clients, getCardNumber(buffer), nr_clients);
									int err = verifyLogin(buffer, clients, nr_clients);
									//se reseteaza contorul pentru incercari gresite la schimbarea cardului
									if (sock_map_lastlog[j] != clients[index].card_number) {
										count = 0;
									}
									//se reseteaza contorul pentru incercari gresite cand un client se logheaza
									if (sock_map_lastlog[j] == clients[index].card_number && err == 1) {
										count = 0;
									}
									//clientul se logheaza
									if (err == 1) {
										clients[index].logged = 0;
										sock_map_logged[j] = clients[index].card_number;
										sprintf(buffer, "ATM> Welcome %s %s\n", clients[index].last_name, clients[index].first_name);
									}
									//pinul este gresit
									else if (err == -3) {
										//s-a gresit de 2 ori
										if (count == 2) {
											getError(buffer, "atm", -5);
											clients[index].blocked = 0;
										}
										else {
											getError(buffer, "atm", err);
											sock_map_lastlog[j] = clients[index].card_number;
										}
										count++;
										
									}
									else {
										getError(buffer, "atm", err);	
									}
									
								}
								//primesc cerere de logout
								if (strcmp(buffer, "Solicit deconectarea!") == 0) {
									printf("%s\n", buffer);
									int k;
									loggedClient.card_number = sock_map_logged[j];
									for (k = 0; k < nr_clients; k++) {
										if (loggedClient.card_number == clients[k].card_number) {
											clients[k].logged = 1;
											sprintf(buffer, "ATM> Deconectare de la bancomat!\n");
											printf("ATM> Deconectez clientul %d\n", loggedClient.card_number);
											break;
										}
									}
									
								}
								if (strcmp(buffer, "listsold") == 0) {
									int k;
									loggedClient.card_number = sock_map_logged[j];
									for (k = 0; k < nr_clients; k++) {
										if (loggedClient.card_number == clients[k].card_number) {
											sprintf(buffer, "ATM> %.2f\n", clients[k].sold);
											break;
										}
									}
									
									
								}
								if (strcmp(command, "getmoney") == 0) {
									int value = (int)getValue(buffer);
									loggedClient.card_number = sock_map_logged[j];
									if(value % 10 != 0) {
										getError(buffer, "atm", -9);
									}
									else {
										int k;
										for (k = 0; k < nr_clients; k++) {
											if (loggedClient.card_number == clients[k].card_number) {
												if (clients[k].sold < value) {
													getError(buffer, "atm", -8);
												}
												else {
													sprintf(buffer, "ATM> Suma %d retrasa cu succes\n", value);
													clients[k].sold = clients[k].sold - value;
												}
												break;
											}
										}	
									}
								}
								if (strcmp(command, "putmoney") == 0) {
									double value = getValue(buffer);
									int k;
									loggedClient.card_number = sock_map_logged[j];
									for (k = 0; k < nr_clients; k++) {
										if (loggedClient.card_number == clients[k].card_number) {
											sprintf(buffer, "ATM> Suma depusa cu succes\n");
											clients[k].sold = clients[k].sold + value;
											break;
										}
									}	
								}
								//primesc cerere de inchidere a conexiunii
								if (strcmp(buffer, "Solicit terminarea conexiunii") == 0) {
									printf("%s\n", buffer);
									int k;
									loggedClient.card_number = sock_map_logged[j];
									//verific daca este un client logat si il deconectez
									for (k = 0; k < nr_clients; k++) {
										if (loggedClient.card_number == clients[k].card_number) {
											clients[k].logged = 1;
											break;
										}
									}
									//inchid socketul si il scot din multime
									close(j); 
									FD_CLR(j, &read_fds);
									break;

								}
								int s = send(j, buffer, strlen(buffer), 0);
								if (s < 0) {
									printf("Eroare trimitere mesaj\n");
								}
							}
						}
					}
				} 
			}
		}
    }
    close(fd);
   
    return 0; 
}