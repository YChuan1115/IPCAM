/*  Make the necessary includes and set up the variables.  */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include "msg.h"
int open_unix_socket(char *path);
int send_cmd (int sockfd,int cmd,int h,int v);
int degree=-1;
int main(int argc,char **argv)
{
    int sockfd;
       int ret; 
   int h=-1,v=-1,cmd;
   

     if (argc==2  ) {
             
              if (strcmp(argv[1],"left")==0)  {
                   cmd=0xAF02;
              } else if (strcmp(argv[1],"right")==0) {
                   cmd=0xAF03;
              } else if (strcmp(argv[1],"up")==0) {
                    cmd=0xAF06;
              } else if(strcmp(argv[1],"down")==0) {
                    cmd=0xAF05;	     
   
              } else  {   
                 cmd =strtol(argv[1],NULL,16);
              }
     
      } else if (argc==3  ) {

            cmd =strtol(argv[1],NULL,16);
            degree =atoi(argv[2]);
     
      } else if (argc ==4) {

            cmd =strtol(argv[1],NULL,16);
   	    degree =atoi(argv[2]);
     	    v =atoi(argv[3]);
     } else {

      printf("%s <cmd>0xAF0[1,4] [<degree>\n",argv[0]);
      printf("%s <cmd>0xAF07[ <horizontal>] <vertical>\n",argv[0]);
      printf("%s <cmd>0xAF0[2,3,5,6][\n",argv[0]);
      printf("example: %s 0xAF07 50  180\n",argv[0]);
      printf("example: %s 0xAF0[1,4] 35\n",argv[0]);
      printf("example: %s 0xAF0[2,3,5,6] \n",argv[0]);
 
      
      return 0;

    }

   
    sockfd=open_unix_socket(Servo_Socket);

     ret=send_cmd(sockfd,cmd,h,v);
   if (ret<0) 
     printf(" send to server failed\n");
   //else 
     //printf(" send to server successufly with %d bytes\n",ret);
  
    close(sockfd);
    exit(0);
}

int send_cmd (int sockfd,int cmd,int h,int v)
{
   struct msg_control msg;
   h=degree;
   msg.cmd=cmd;
   msg.angle.vertical=v;
   msg.angle.horizontal=h;

   // printf(" send to server: %#x %d %d\n",cmd,h,v);
   
   return write(sockfd, &msg, sizeof(struct msg_control));

}




int open_unix_socket(char *path)
{ 

    int sockfd;
    int len;
    struct sockaddr_un address;
    int result;
/*  Create a socket for the client.  */

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

/*  Name the socket, as agreed with the server.  */

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path);
    len = sizeof(address);

/*  Now connect our socket to the server's socket.  */

    result = connect(sockfd, (struct sockaddr *)&address, len);

    if(result == -1) {
        perror("oops: client");
        exit(1);
    }

   return sockfd;

}

