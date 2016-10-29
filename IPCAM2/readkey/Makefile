
CC=gcc
#CC=arm-linux-gcc
CFLAG=-lbcm2835

all: ipcam josephtest

ipcam: ipcam_server.c unixsock.c unixsock.h msg.h
	$(CC) ipcam_server.c unixsock.c -o $@ $(CFLAG)
	


josephtest: client_unix.c msg.h
	$(CC) client_unix.c -o $@


clean:
	rm -rf servo_socket ipcam josephtest
	
