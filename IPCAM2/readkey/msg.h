

#ifndef __msg_h__

  #define  __msg_h__

 typedef struct {
  int horizontal;
  int vertical;

}Angle;

#define CMD_PAN_SET		0xAF01
#define CMD_PAN_PLUS 		0xAF02
#define CMD_PAN_MINUS 		0xAF03
#define CMD_TILT_SET 		0xAF04
#define CMD_TILT_PLUS 		0xAF05
#define CMD_TILT_MINUS 		0xAF06
#define CMD_PAN_TILT_SET 	0xAF07
#define Servo_Socket "/tmp/servo_socket"
 struct msg_control {

  	 int cmd;
         Angle angle;
	  char arg;
        int numarg;
   
 };



 enum STATUS{
   OK=0,
   ERROR,
   UNKNOWN,
};



 struct msg_report {
     enum STATUS status; 
     int error_code;
     char data[16];


 };

#endif
 
