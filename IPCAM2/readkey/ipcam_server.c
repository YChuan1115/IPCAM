#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "msg.h"
#include "unixsock.h"
#include"pca9685.h"
//#include "dc_motor.h"

#define DEVICE_BLTEST "/dev/servo0"      

int pwm_w0=1480,pwm_w1=1480;

#define ARM


#define POLLTIME 10000000// us
//#ifdef ARM
//#define KEYPAD_DEVICE "/dev/event0"     // ????????????? // /event0 是幹啥用的?
//#else 
//#define KEYPAD_DEVICE "/dev/input/event0"
//#endif
#define KEYPAD_DEVICE "/dev/input/event0"


char get_keycode(int fd);
void handle_keyevent(char key);



int turn_left(void *arg);
int turn_right(void *arg);
int turn_up(void *arg);
int turn_down(void *arg);
int set_pwm(int h,int v);
int set_pwm_bmc2835(int h,int v);
int restart(void *arg);

typedef int (*EV_Handler) (void * arg);

typedef struct {
  char keycode;
  EV_Handler handler;
} KEY_ACTION_MAP;

#define KEY1  115 
#define KEY2  105
#define KEY3  106
#define KEY4  229
#define KEY5  114
#define KEY6  158
#define KEY7  28
#define KEY8  107
#define KEYNUM 8

#define  MIN(a,b) ((a)<(b)?(a):(b))
#define  MAX(a,b) ((a)>(b)?(a):(b))
#define  BUF_SIZE 128 
#define MOVE_STEP 10

#define PWM_DEVICE "/dev/servo0"   
#define I2C_SLAVE_ADDR 0x40
#define BOTTOM_MOTOR_CHANNEL 1
#define TOP_MOTOR_CHANNEL 2

void process_data(char *buf,int size);

KEY_ACTION_MAP key_action[KEYNUM]={
  {KEY1,restart},
  {KEY2,NULL},
  {KEY3,turn_up},
  {KEY4,NULL},
  {KEY5,NULL},
  {KEY6,turn_left},
  {KEY7,turn_down},
  {KEY8,turn_right},
};


//global data fir angle
Angle angle;
 int fd_pwm; //for pwm device

int main(int argc, char *argv[]) {
	fd_set rfds,wfds;
 
	struct timeval tv;
	struct msg_control msg;  // control message 
	int retval;
	char *device = KEYPAD_DEVICE; //default , device name
	int idx;
	int fd_key;
	int sock;
	char buf[BUF_SIZE];
	int newsock,maxsock;
 
  //int amount_client;
  char name[256] = "Unknown";
  angle.horizontal=0;
  angle.vertical=-30;
  int i;
  
	if (argc > 1) device = argv[1]; // if argv[1] , this indicate the device


               

	//
	// open PWM device
	// 
	//fd_pwm = open(PWM_DEVICE,O_RDONLY);  // open the device 
	//if(fd_pwm<0 )
	//{
	//	fprintf(stderr,"can not open device %s\n",PWM_DEVICE);
	//	exit(1);
	//}               
	pca9685_init(I2C_SLAVE_ADDR, 50);
	motor_turn(angle.horizontal,angle.vertical);

	// END OPEN PWM DEVICE


	//
	// START to Init the Socket Part
	// open unix domain socket
   unlink(Servo_Socket);

   for (i=0;i<MAX_CLIENT;i++) 
        client_sock[i]=0;
   sock = open_socket( Servo_Socket);

   if (sock<0) 
       printf ("cannot create Unix socket\n");

   maxsock=fd_key;

   if ((getuid ()) != 0)
    printf ("You are not root! This may not work...\n");
 


   //
   //  Open Device
   //
  if ((fd_key = open (device, O_RDONLY)) == -1) {
      printf ("%s is not a vaild device\n", device);
         return -1;
  } 

  
 
  //Print Device Name // 得到 /dev/input/event0 所代表的device name
  if (ioctl (fd_key, EVIOCGNAME (sizeof (name)), name) ==-1 ) {
           perror("error");
           return -1;
   }
 
     printf ("Reading From : %s (%s)\n", device, name);
    /* Watch stdin (fd 0) to see when it has input. */
   
    maxsock=fd_key;

    FD_ZERO(&rfds); // 清空準備要用在select 裡面的 read file descriptor list
    FD_ZERO(&wfds); // 清空準備要用在select 裡面的 write file descriptor list
 
  //  FD_SET(0, &rfds);
    FD_SET(sock, &rfds);
    FD_SET(sock, &wfds);
    FD_SET(fd_key, &rfds);

  fprintf(stderr,"===Start PanTilt IPCAM Server====\n");
  while (1) {

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
 
   // FD_SET(0, &rfds);
    FD_SET(sock, &rfds);
    FD_SET(sock, &wfds);
    FD_SET(fd_key, &rfds);

  
     for (i=0;i<MAX_CLIENT;i++) {
       if (client_sock[i]!=0) {
         FD_SET(client_sock[i], &rfds);
       }
     }
  
    /* Wait up to five seconds. */
    tv.tv_sec = 0;
    tv.tv_usec = POLLTIME;
  
   retval = select(maxsock+1, &rfds,NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */
    //printf("select retval=%d\n",retval);
    if (retval == -1)
        perror("select()");
    else if  (retval == 0) {
        // printf("select timeout\n");
	continue ;
    } 
 

      // handle keypad event
        if (FD_ISSET(fd_key, &rfds)) {
               char key=get_keycode(fd_key);                 
                handle_keyevent(key);


         } 
       //chehck new connection  ---------------
         if (FD_ISSET(sock, &rfds)) {
          
              newsock = accept(sock,NULL,0);    
              printf( "new client [%d]\n " , newsock); 
	     if (newsock != -1) { 
                   //  add to client socket queue 
                 if  ( (idx=get_free_idx())!=-1) { 
                   client_sock[idx]  =  newsock ; 
                   printf( "new client conneting client[%d]\n " , idx); 
             
                     if  (newsock >  maxsock)  
				maxsock  = newsock; 
                      
                } else  { 
            
              	   printf("over max connections, exit!\n ");              	   
                   close(newsock); 
                   break ; 
                  }  
            
             } else  {
                 perror ("accept():");
			
             }

         } 
      //--------------------------------------------------------
     
          //  check every socket in the set 
        for  (i  =  0 ; i  <  MAX_CLIENT; i ++ ) { 
            if  (FD_ISSET(client_sock[i],  & rfds)) { 
                retval  =  recv(client_sock[i], buf,  sizeof (struct msg_control),  0 ); 
                if  (retval  <=  0 ) {         //  client close 
                      
                     printf("client[%d] close\n",i); 
                     
                    close(client_sock[i]); 
                     
                    FD_CLR(client_sock[i],&rfds); 
                    
                    client_sock[i]  =  0 ; 
                  
                }  else  {  //   process msg data 
           
		        printf("Prepare to processing data\n"); 
                        process_data(buf,retval);
	 	
                } 
            } 
        } 

    //-------------------------------------------
    // debug 
   //  showclient(); 
   
  
 

   } //end while (1)

    close(fd_key);
    close_socket(sock);
     //  close all client connections 
    for  (i  =  0 ; i  <  MAX_CLIENT; i ++ ) { 
        if  (client_sock[i]  !=  0 ) { 
            close(client_sock[i]); 
        } 
    }  
    unlink(Servo_Socket);

    return 0;
}

void process_data(char *buf,int size)
{
      int cmd,v,h;
     struct msg_control *pmsg=(struct msg_control *)buf;
      

      
      cmd=pmsg->cmd;
     
     printf("ipcam server got cmd => %#x %d %d\n",pmsg->cmd,pmsg->angle.horizontal,pmsg->angle.vertical);

   switch (cmd) {
      case CMD_PAN_SET:
	   fprintf(stderr,"OUT_CMD_PAN_SET\n"); 
	   angle.horizontal=pmsg->angle.horizontal;
	   h=angle.horizontal;
	   v=angle.vertical;
	   motor_turn(h,v);
	   break;
      case CMD_PAN_PLUS:
	   fprintf(stderr,"CMD_PAN_PLUS\n"); 
	   turn_left(NULL);
	   
	   break;
      case CMD_PAN_MINUS:
	   fprintf(stderr,"CMD_PAN_MINUS\n"); 
	   turn_right(NULL);
	   break;
      case CMD_TILT_SET:
	   fprintf(stderr,"CMD_TILT_SET\n"); 
	   angle.vertical=pmsg->angle.horizontal;//!!!! USE  this to turn
           v=angle.vertical;
	   h=angle.horizontal;	   	
	   motor_turn(h,v);
	   break;
      case CMD_TILT_PLUS:
	   fprintf(stderr,"CMD_TILT_PLUS\n"); 
	   turn_up(NULL);
	   break;
      case CMD_TILT_MINUS:
	   fprintf(stderr,"CMD_TILT_MINUS\n"); 
	   turn_down(NULL);
	   break;
      case CMD_PAN_TILT_SET:
	   fprintf(stderr,"CMD_PAN_TILT_SET\n"); 
	   v=pmsg->angle.vertical;
	   h=pmsg->angle.horizontal;
	   motor_turn(h,v);
	   break;
	default: 
          fprintf(stderr,"ipcam daemon: Unknow Command!\n");
     }


      
   
}




char get_keycode(int fd)
{

   struct input_event ev;
  int rd, value, size = sizeof (struct input_event);
 
   if ((rd = read (fd, &ev, size)) < size)
         perror ("read from key:");      

    if (ev.type == 1 && (ev.value==2 || ev.value==0)) {//value=> 1: key pressed, 0: release key : 2: key presseed continuously....
	   // printf ("Code=%d,value=%d\n", ev.code,ev.value);
      return ev.code;  
	}    else 
      return 0;
}

void handle_keyevent(char key)
{

 
   //handle key
  int i;
  for(i=0;i<KEYNUM;i++)  
   if (key==key_action[i].keycode && key_action[i].handler!=NULL) {
     //printf ("`11key Code=%d\n", key); 
     key_action[i].handler(NULL);
   }

 }

int turn_up(void *arg)
{
  angle.vertical=MIN(angle.vertical+ MOVE_STEP,50);
  motor_turn(angle.horizontal,angle.vertical);
}

int turn_down(void *arg)
{
  // angle.vertical-=10;
  angle.vertical=MAX(angle.vertical- MOVE_STEP,-80);
  motor_turn(angle.horizontal,angle.vertical);

}

int turn_left(void *arg)
{
   angle.horizontal=MIN(angle.horizontal+ MOVE_STEP,90);
    motor_turn(angle.horizontal,angle.vertical);
}

int turn_right(void *arg)
{
 
 // angle.horizontal-=10;
   angle.horizontal=MAX(angle.horizontal- MOVE_STEP,-90);
    motor_turn(angle.horizontal,angle.vertical);
}


int motor_turn(int h, int v)
{
  set_pwm_bmc2835(h,v);
}

int restart(void *arg)
{
 
   system("restart.sh");
}



		
            
/*             
  */          
   //  ========================= IPCAM for Web UI COMMAND ========================
   //  han-hsian 2012/11/20
   // 
   //  refer to output_httpd.c, output.h, httpd.h (modify relative OUT_CMD_XXX )
   //
   //  0 - 180 servo motor control by angle 
   //
   //  channel 0 = bottom motor 
   //  channel 1 = top motor
   //  
   //  channel 0 (bottom motor prameter)  
   //  Angle 0 = pwm_pulse 500 , Angle 180 =- pwm_pulse  2450 , Angle Reset = 1300 (about 90)                     
   //  per angle = (2450 - 500) / 180 degree = 10.833 pulse/degree 
   

   //  channel 1 (top motor) 
   //  Angle 0 =  pwm_pulse 540 , angle 180 = pwm_pulse 1680 , Angele Reset = 850 (about 90)
   //  per angle = (1680 - 540) / 180 degree = 6.33 pulse/degree  
                
   //==================== Initial servo motor to angle 90 degree ==============
                
   // top_motor_angle = 850;        // initial servo top motor 
   // bottom_motor_angle = 1300;    // initial servo bottom motor
 
  
/*
 * INPUT:
 * 		int h : the angle set of bottom motor (unit: deg.)
 * 		int v : the angle set of top    motor (unit: deg.)
 *
 * ACTION: 
 * 		move the angle of bottom motor to h deg.
 * 		move the angle of top motor to v deg.
 *
 * return : ? 
 */
//int set_pwm(int h,int v)
//{   
//    
//	PWM_PARAM pwm;  			// member, int channel , int pulse_width
//	int top_motor_angle ;
//	int bottom_motor_angle ;
//
//	//
//	// ===== set bottom motor ====  
//	//
//	bottom_motor_angle =  500 + h * ( 2450 - 500 ) /180;   // result is the count of the pwm pulse width , h is the angle (uint deg.)
//
//	pwm.channel=0;
//	pwm_w0 = bottom_motor_angle;
//	pwm.pulse_width=pwm_w0;
//	ioctl(fd_pwm,DC_IOCTL_CLOCKWISE,&pwm);
//
//	//
//	// ============== set top motor ===============  
//	//
//	top_motor_angle = 540 +  v* (1680 - 540) / 180 ; 
//         
//	pwm.channel=1;
//	pwm_w0=top_motor_angle;
//	pwm.pulse_width=pwm_w0;
//	ioctl(fd_pwm,DC_IOCTL_CLOCKWISE,&pwm);
//}

// input: 
//     h is the angle for bottom motor unit in deg.
//     v is the angle for top motor unit in deg.
//
int set_pwm_bmc2835(int h,int v)
{   
	//
	// ===== set bottom motor ====  
	//
	float dcyc = h * 9./180. + 7.5;
	pca9685_setDutyCycle(I2C_SLAVE_ADDR, BOTTOM_MOTOR_CHANNEL,  dcyc);

	//
	// ============== set top motor ===============  
	//
	dcyc = v * 9./180. + 7.5;
	pca9685_setDutyCycle(I2C_SLAVE_ADDR, TOP_MOTOR_CHANNEL,  dcyc);
}


