#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "lib.h"

void error(char *msg)
{
    printf("%s\n",msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n, i, sockudp;
    struct sockaddr_in serv_addr, my_sockaddr;
    struct hostent *server;
    fd_set read_fds;    //multimea de citire folosita in select()
    fd_set tmp_fds;    //multime folosita temporar 
    int fdmax;     //valoare maxima file descriptor din multimea read_fds

    char command[LEN], answer[LEN];
    char* filename = malloc(100 * sizeof(char));
    sprintf(filename, "%s-%ld.%s", "client", (long)getpid(), "log");
    int fd = open (filename , O_RDWR | O_CREAT, 0666); 

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds); 

    sockudp = socket(PF_INET, SOCK_DGRAM, 0); //creare socket udp
    if (sockudp < 0) 
        error("-10 : Eroare de apel socket\n");
    //introducere descritor nou(socket udp)
    FD_SET(sockudp, &read_fds);

    //initializare structura udp
    my_sockaddr.sin_family = AF_INET;
    my_sockaddr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &my_sockaddr.sin_addr);

	sockfd = socket(AF_INET, SOCK_STREAM, 0); //creare socket tcp
    if (sockfd < 0) 
        error("-10 : Eroare de apel socket\n");

    //initializare structura tcp
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("-10 : Eroare de apel connect\n");  

    //introducere descriptor nou(socket tcp)
    FD_SET(sockfd, &read_fds);

    //introducere descriptor pentru citit de la tastatura
    FD_SET(0, &read_fds);

    if(sockfd > sockudp) {
        fdmax = sockfd;
     }
     else{
        fdmax = sockudp;
     }

    int err = 0;
    char errmsg[LEN];
    int quit = 0;
    int last_card_number;
    int secret_password = 0;
    while (1) {
        if (quit == 1) {
            break;
        }
        tmp_fds = read_fds; 
        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
            error("-10 : Eroare de apel select\n");
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == sockudp) {
                    //primesc pe socketul udp
                    memset(answer, 0, LEN);
                    socklen_t len = sizeof(my_sockaddr);
                    if ((n = recvfrom(i, answer, LEN, 0, (struct sockaddr*)&my_sockaddr, &len)) <= 0) {
                        if (n == 0) {
                            //conexiunea s-a inchis
                            printf("client: socket %d hung up\n", i);
                        } else {
                            error("ERROR in recv");
                        }
                    }
                    else {
                        //primesc cerere sa trimit parola secreta
                        if (strcmp(answer, "UNLOCK> Trimite parola secreta\n") == 0) {
                            printf("%s\n", answer);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, answer, strlen(answer));
                            write(fd, "\n", strlen("\n"));
                            secret_password = 1;  
                        }
                        else {
                            printf("%s\n", answer);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, answer, strlen(answer));
                            write(fd, "\n", strlen("\n"));
                            secret_password = 0; 
                        }
                        
                    }   
                }
            
                if (i == sockfd) {
                    //primesc pe socketul tcp
                    memset(answer, 0, LEN);
                    if ((n = recv(i, answer, sizeof(answer), 0)) <= 0) {
                        if (n == 0) {
                            //conexiunea s-a inchis
                            printf("client: socket %d hung up\n", i);
                        } else {
                            error("ERROR in recv");
                        }
                    }    
                    else {
                        //am primit confirmarea de autentificare cu succes
                        if (clientIsLogged(answer) == 0) {
                            err = -2;
                        }
                        //primesc notificare din partea serverului ca se va inchide
                        if (strcmp(answer, "Serverul se va deconecta!\n") == 0) {
                            printf("%s\n", answer);
                            quit = 1;
                            break;  
                        }
                        else {
                            printf("%s\n", answer);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, answer, strlen(answer));
                            write(fd, "\n", strlen("\n"));  
                        }
                        
                    }    
                }

                if (i == 0) {
                    //scriu comanda de la tastatura
                    memset(command, 0 , LEN);
                    fgets(command, LEN-1, stdin);
                    command[strlen(command) - 1] = '\0';
                    char *tok = getCommand(command);

                    if (strcmp(tok, "login") == 0) {
                        last_card_number = getCardNumber(command);
                        if (err == -2) {
                            getError(errmsg, "client", err);
                            printf("%s\n", errmsg);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, errmsg, strlen(errmsg));
                            write(fd, "\n", strlen("\n"));
                            continue;
                        }
                    }
                    if (strcmp(command, "logout") == 0) {
                        char *temp = strdup(command);
                        if(err == -2) {
                            sprintf(temp, "Solicit deconectarea!");
                            n = send(sockfd, temp, strlen(temp), 0);
                            if (n < 0) 
                                error("ERROR writing to socket");
                            err = 0;
                            continue;
                        }
                        else {
                            getError(errmsg, "client", -1);
                            printf("%s\n", errmsg);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, errmsg, strlen(errmsg));
                            write(fd, "\n", strlen("\n"));
                            continue;
                        }
                    }

                    if (strcmp(command, "listsold") == 0) {
                        if (err != -2) {
                            getError(errmsg, "client", -1);
                            printf("%s\n", errmsg);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, errmsg, strlen(errmsg));
                            write(fd, "\n", strlen("\n"));
                            continue;    
                        }
                    }

                    if (strcmp(tok, "getmoney") == 0) {
                        if (err != -2) {
                            getError(errmsg, "client", -1);
                            printf("%s\n", errmsg);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, errmsg, strlen(errmsg));
                            write(fd, "\n", strlen("\n"));
                            continue;    
                        }     
                    }

                    if (strcmp(tok, "putmoney") == 0) {
                        if (err != -2) {
                            getError(errmsg, "client", -1);
                            printf("%s\n", errmsg);
                            write(fd, command, strlen(command));
                            write(fd, "\n", strlen("\n"));
                            write(fd, errmsg, strlen(errmsg));
                            write(fd, "\n", strlen("\n"));
                            continue;    
                        }   
                    }

                    if (strcmp(command, "quit") == 0) {
                        char *temp = strdup(command);
                        sprintf(temp, "Solicit terminarea conexiunii");
                        write(fd, command, strlen(command));
                        write(fd, "\n", strlen("\n"));
                        n = send(sockfd, temp, strlen(temp), 0);
                        if (n < 0) 
                            error("ERROR writing to socket");
                        quit = 1;
                        break;
                        
                    }

                    if (strcmp(tok, "unlock") == 0) {
                        char *temp = strdup(command);
                        sprintf(temp, "%s %d", command, last_card_number);
                        n = sendto(sockudp, temp, strlen(temp), 0, (struct sockaddr*)&my_sockaddr, sizeof(my_sockaddr));
                        if (n < 0) {
                            error("ERROR writing to socket");
                        }
                        continue;
                    }

                    if (secret_password == 1) {
                        char *temp = strdup(command);
                        sprintf(temp, "%d %s", last_card_number, command);
                        n = sendto(sockudp, temp, strlen(temp), 0, (struct sockaddr*)&my_sockaddr, sizeof(my_sockaddr));
                        if (n < 0) {
                            error("ERROR writing to socket");
                        }
                        continue;
                    }
                    n = send(sockfd, command, strlen(command), 0);
                    if (n < 0) 
                        error("ERROR writing to socket");  
                }
            }        
        }        
    }
    //inchid socketii pentru comunicarea udp, tcp si desciptorul de fisier
	close(sockfd);
    close(sockudp);
    close(fd);

    return 0;
}


