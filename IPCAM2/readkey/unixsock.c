/*  Make the necessary includes and set up: the variables.  */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include "unixsock.h"

  int client_sock[MAX_CLIENT];
  int client_sock_idx=0;

int open_socket(char *sock_path)
{
    int server_sockfd, client_sockfd;
    int server_len;
    int ret;
    struct sockaddr_un server_address;


/*  Remove any old socket and create an unnamed socket for the server.  */

   
    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sockfd <0) {
       perror("socket:");
      return -1;
   
    }

/*  Name the socket.  */

    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path,sock_path);
    server_len = sizeof(server_address);
    ret=bind(server_sockfd, (struct sockaddr *)&server_address, server_len);
     if (ret <0) { 
        perror("socket:");
      return -1;

    }


/*  Create a connection queue and wait for clients.  */

    listen(server_sockfd, 5);
    int yes=1;
   if  (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR,  
        & yes,  sizeof ( int ))  ==  -1 ) { 
        perror( " setsockopt " ); 
        exit( 1 ); 
    }  
    return server_sockfd;

}

int close_socket(int sock)
{
 close(sock);

}

void  showclient() 
{ 
    int  i; 
    int tot=0;
    for  (i  =  0 ; i  <  MAX_CLIENT; i ++ ) { 
        printf("[%d]:%d\n" , i, client_sock[i]); 
        if ( client_sock[i]) tot++;
    } 
   printf( "Amount of client:%d\n",tot); 
} 

// get free client socket  index
int  get_free_idx() 
{ 
    int  i; 
    
    for  (i  =  0 ; client_sock[i] !=0 && i  <  MAX_CLIENT ; i ++ ) { 
        printf( " [%d]:%d   " , i, client_sock[i]); 
    } 
    return (i< MAX_CLIENT)? i : -1 ;
   
} 


