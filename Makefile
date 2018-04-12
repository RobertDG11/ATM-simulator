CC=gcc
LIBSOCKET=-lnsl
CCFLAGS=-Wall -g
SRV=server
CLT=client
AUX = aux

all: $(SRV) $(CLT)

$(SRV):$(SRV).o $(AUX).o
	$(CC) -o $(SRV) $(SRV).o $(AUX).o

$(CLT):	$(CLT).o $(AUX).o
	$(CC) -o $(CLT) $(CLT).o $(AUX).o

.c.o:
	$(CC) -c $(LIBSOCKET) $(SRV).c $(CLT).c $(AUX).c

clean:
	rm -f *.o *~
	rm -f $(SRV) $(CLT)

cleanlog:
	rm -f *.log
