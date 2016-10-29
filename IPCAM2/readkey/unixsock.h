#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef __unixsock_h__

  #define  __unixsock_h__

 #define MAX_CLIENT 10

  extern int client_sock[];
  extern int client_sock_idx;

int open_socket(char *sock_path);
int  get_free_idx() ;
void  showclient() ;

#endif
