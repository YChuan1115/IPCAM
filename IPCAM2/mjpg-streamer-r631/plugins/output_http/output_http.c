/*******************************************************************************
#                                                                              #
#      MJPG-streamer allows to stream JPG frames from an input-plugin          #
#      to several output plugins                                               #
#                                                                              #
#      Copyright (C) 2007 Tom Stöveken                                         #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; version 2 of the License.                      #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <pthread.h>
#include <syslog.h>

#include "../../mjpg_streamer.h"
#include "../../utils.h"
#include "httpd.h"

#define OUTPUT_PLUGIN_NAME "HTTP output plugin"
#define MAX_ARGUMENTS 32

/*
 * keep context for each server
 */
context servers[MAX_OUTPUT_PLUGINS];

/******************************************************************************
Description.: print help for this plugin to stdout
Input Value.: -
Return Value: -
******************************************************************************/
void help(void) {
  fprintf(stderr, " ---------------------------------------------------------------\n" \
                  " Help for output plugin..: "OUTPUT_PLUGIN_NAME"\n" \
                  " ---------------------------------------------------------------\n" \
                  " The following parameters can be passed to this plugin:\n\n" \
                  " [-w | --www ]...........: folder that contains webpages in \n" \
                  "                           flat hierarchy (no subfolders)\n" \
                  " [-p | --port ]..........: TCP port for this HTTP server\n" \
                  " [-c | --credentials ]...: ask for \"username:password\" on connect\n" \
                  " [-n | --nocommands ]....: disable execution of commands\n"
                  " ---------------------------------------------------------------\n");
}

/*** plugin interface functions ***/
/******************************************************************************
Description.: Initialize this plugin.
              parse configuration parameters,
              store the parsed values in global variables
Input Value.: All parameters to work with.
              Among many other variables the "param->id" is quite important -
              it is used to distinguish between several server instances
Return Value: 0 if everything is OK, other values signal an error
******************************************************************************/
int output_init(output_parameter *param) {
  char *argv[MAX_ARGUMENTS]={NULL};
  int  argc=1, i;
  int  port;
  char *credentials, *www_folder;
  char nocommands;

  DBG("output #%02d\n", param->id);

  port = htons(8080);
  credentials = NULL;
  www_folder = NULL;
  nocommands = 0;

  /* convert the single parameter-string to an array of strings */
  argv[0] = OUTPUT_PLUGIN_NAME;
  if ( param->parameter_string != NULL && strlen(param->parameter_string) != 0 ) {
    char *arg=NULL, *saveptr=NULL, *token=NULL;

    arg=(char *)strdup(param->parameter_string);

    if ( strchr(arg, ' ') != NULL ) {
      token=strtok_r(arg, " ", &saveptr);
      if ( token != NULL ) {
        argv[argc] = strdup(token);
        argc++;
        while ( (token=strtok_r(NULL, " ", &saveptr)) != NULL ) {
          argv[argc] = strdup(token);
          argc++;
          if (argc >= MAX_ARGUMENTS) {
            OPRINT("ERROR: too many arguments to output plugin\n");
            return 1;
          }
        }
      }
    }
  }

  /* show all parameters for DBG purposes */
  for (i=0; i<argc; i++) {
    DBG("argv[%d]=%s\n", i, argv[i]);
  }

  reset_getopt();
  while(1) {
    int option_index = 0, c=0;
    static struct option long_options[] = \
    {
      {"h", no_argument, 0, 0},
      {"help", no_argument, 0, 0},
      {"p", required_argument, 0, 0},
      {"port", required_argument, 0, 0},
      {"c", required_argument, 0, 0},
      {"credentials", required_argument, 0, 0},
      {"w", required_argument, 0, 0},
      {"www", required_argument, 0, 0},
      {"n", no_argument, 0, 0},
      {"nocommands", no_argument, 0, 0},
      {0, 0, 0, 0}
    };

    c = getopt_long_only(argc, argv, "", long_options, &option_index);

    /* no more options to parse */
    if (c == -1) break;

    /* unrecognized option */
    if (c == '?'){
      help();
      return 1;
    }

    switch (option_index) {
      /* h, help */
      case 0:
      case 1:
        DBG("case 0,1\n");
        help();
        return 1;
        break;

      /* p, port */
      case 2:
      case 3:
        DBG("case 2,3\n");
        port = htons(atoi(optarg));
        break;

      /* c, credentials */
      case 4:
      case 5:
        DBG("case 4,5\n");
        credentials = strdup(optarg);
        break;

      /* w, www */
      case 6:
      case 7:
        DBG("case 6,7\n");
        www_folder = malloc(strlen(optarg)+2);
        strcpy(www_folder, optarg);
        if ( optarg[strlen(optarg)-1] != '/' )
          strcat(www_folder, "/");
        break;

      /* n, nocommands */
      case 8:
      case 9:
        DBG("case 8,9\n");
        nocommands = 1;
        break;
    }
  }

  servers[param->id].id = param->id;
  servers[param->id].pglobal = param->global;
  servers[param->id].conf.port = port;
  servers[param->id].conf.credentials = credentials;
  servers[param->id].conf.www_folder = www_folder;
  servers[param->id].conf.nocommands = nocommands;

  OPRINT("www-folder-path...: %s\n", (www_folder==NULL)?"disabled":www_folder);
  OPRINT("HTTP TCP port.....: %d\n", ntohs(port));
  OPRINT("username:password.: %s\n", (credentials==NULL)?"disabled":credentials);
  OPRINT("commands..........: %s\n", (nocommands)?"disabled":"enabled");

  return 0;
}

/******************************************************************************
Description.: this will stop the server thread, client threads
              will not get cleaned properly, because they run detached and
              no pointer is kept. This is not a huge issue, because this
              funtion is intended to clean up the biggest mess on shutdown.
Input Value.: id determines which server instance to send commands to
Return Value: always 0
******************************************************************************/
int output_stop(int id) {

  DBG("will cancel server thread #%02d\n", id);
  pthread_cancel(servers[id].threadID);

  return 0;
}

/******************************************************************************
Description.: This creates and starts the server thread
Input Value.: id determines which server instance to send commands to
Return Value: always 0
******************************************************************************/
int output_run(int id) {
  DBG("launching server thread #%02d\n", id);

  /* create thread and pass context to thread function */
  pthread_create(&(servers[id].threadID), NULL, server_thread, &(servers[id]));
  pthread_detach(servers[id].threadID);

  return 0;
}

/******************************************************************************
Description.: This is just an example function, to show how the output
              plugin could implement some special command.
              If you want to control some GPIO Pin this is a good place to
              implement it. Dont forget to add command types and a mapping.
Input Value.: cmd is the command type
              id determines which server instance to send commands to
Return Value: 0 indicates success, other values indicate an error
******************************************************************************/
  //added by joseph , 2012,Nov 26
#define CMD_PAN_SET		0xAF01
#define CMD_PAN_PLUS 		0xAF02
#define CMD_PAN_MINUS 		0xAF03
#define CMD_TILT_SET 		0xAF04
#define CMD_TILT_PLUS 		0xAF05
#define CMD_TILT_MINUS 		0xAF06
#define CMD_PAN_TILT_SET 	0xAF07
//-------------------


int output_cmd(int id, out_cmd_type cmd, int value) {

  //char tmpcmd[50] = "/bin/IPCAM_motor_Web "; // 2012-11-20 hsian  
  fprintf(stderr, "command (%d, value: %d) triggered for plugin instance #%02d\n", cmd, value, id);
  //added by joseph , 2012,May 21
  char cmdbuf[64];

  switch (cmd) {
       
      case OUT_CMD_HELLO:
		   fprintf(stderr,"test hello command\n");
		   break; 

      case OUT_CMD_LEDON:
		   fprintf(stderr,"test ledon command\n"); 
		   system ("echo 1 > /dev/led");
		 //  system("kill -SIGUSR1 `pidof dhome`");
		   break;

      case OUT_CMD_LEDOFF:
		  system ("echo 6 > /dev/led");
                  fprintf(stderr,"test ledoff command\n");
		   //system("kill -SIGUSR2 `pidof dhome`");
           break;

      case OUT_CMD_DIAL:
		  // system ("echo 6 > /dev/led");
       
		   system("kill -SIGHUP `pidof dhome`");
           break;
      case OUT_CMD_HANGUP:
		  // system ("echo 6 > /dev/led");
        
	     system("kill -SIGWINCH `pidof dhome`");
           break;
      case OUT_CMD_VIDEOON:
		  // system ("echo 6 > /dev/led");
             fprintf(stderr,"go viedo switch command\n");
	     system("kill -SIGPWR `pidof dhome`");
           break;

      case OUT_CMD_VIDEOOFF:
		  // system ("echo 6 > /dev/led");
        
	     system("kill -SIGIO `pidof dhome`");
           break;

      case OUT_CMD_LEDBLINK:
	   system ("echo 5 > /dev/led");
           fprintf(stderr,"test led blink command\n");
           break;		   

// 2012-11-20 han-hsian IPCAM project command added for Web UI
//   
//  get command from web (streammer) 
//  eX:  TOP motor(上下) 0 度 / Botttom motor(左右) 0 度：
//       http://10.110.20.239:8080/?action=command&command=1Q
//  
//  then send command to console 
//  ex   ./IPCAM_motor_Web 1Q 
//
//
// ---------------------------------------------- row 1
#if 0
     case  OUT_CMD_1Q:   
           fprintf(stderr,"1Q\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 1Q ");
            break;
     case  OUT_CMD_2Q:   
           fprintf(stderr,"2Q\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 2Q ");
           break;
     case  OUT_CMD_3Q:   
           fprintf(stderr,"3Q\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 3Q ");
           break;
     case  OUT_CMD_4Q:   
           fprintf(stderr,"4Q\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 4Q ");
           break;
     case  OUT_CMD_5Q:   
           fprintf(stderr,"5Q\n"); 
           system ("/motor_pwm/IPCAM_motor_Web 5Q ");
           break;
// ---------------------------------------------- row 2
     case  OUT_CMD_1W:   
           fprintf(stderr,"1W\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 1W ");
            break;
     case  OUT_CMD_2W:   
           fprintf(stderr,"2W\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 2W ");
           break;
     case  OUT_CMD_3W:   
           fprintf(stderr,"3W\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 3W ");
           break;
     case  OUT_CMD_4W:   
           fprintf(stderr,"4W\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 4W ");
           break;
     case  OUT_CMD_5W:   
           fprintf(stderr,"5W\n"); 
           system ("/motor_pwm/IPCAM_motor_Web 5W ");
           break;
// ---------------------------------------------- row 3
     case  OUT_CMD_1E:   
           fprintf(stderr,"1E\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 1E ");
            break;
     case  OUT_CMD_2E:   
           fprintf(stderr,"2E\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 2E ");
           break;
     case  OUT_CMD_3E:   
           fprintf(stderr,"3E\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 3E ");
           break;
     case  OUT_CMD_4E:   
           fprintf(stderr,"4E\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 4E ");
           break;
     case  OUT_CMD_5E:   
           fprintf(stderr,"5E\n"); 
           system ("/motor_pwm/IPCAM_motor_Web 5E ");
           break;
// ---------------------------------------------- row 4

     case  OUT_CMD_1R:   
           fprintf(stderr,"1R\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 1R ");
            break;
     case  OUT_CMD_2R:   
           fprintf(stderr,"2R\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 2R ");
           break;
     case  OUT_CMD_3R:   
           fprintf(stderr,"3R\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 3R ");
           break;
     case  OUT_CMD_4R:   
           fprintf(stderr,"4R\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 4R ");
           break;
     case  OUT_CMD_5R:   
           fprintf(stderr,"5R\n"); 
           system ("/motor_pwm/IPCAM_motor_Web 5R ");
           break;
// ---------------------------------------------- row 5
     case  OUT_CMD_1T:   
           fprintf(stderr,"1T\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 1T ");
            break;
     case  OUT_CMD_2T:   
           fprintf(stderr,"2T\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 2T ");
           break;
     case  OUT_CMD_3T:   
           fprintf(stderr,"3T\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 3T ");
           break;
     case  OUT_CMD_4T:   
           fprintf(stderr,"4T\n"); 
	   system ("/motor_pwm/IPCAM_motor_Web 4T ");
           break;
     case  OUT_CMD_5T:   
           fprintf(stderr,"5T\n"); 
           system ("/motor_pwm/IPCAM_motor_Web 5T ");
           break;
// ---------------------------------------------- other
     case  OUT_CMD_7X:   
          fprintf(stderr,"7X\n"); 
          system ("/motor_pwm/IPCAM_motor_Web 7X ");
          break;

    case  OUT_CMD_6X:   
          fprintf(stderr,"7X\n"); 
          system ("/motor_pwm/IPCAM_motor_Web 6X ");
          break;

    case  OUT_CMD_XY:   
          fprintf(stderr,"XY\n"); 
          system ("/motor_pwm/IPCAM_motor_Web XY ");
          break;

    case  OUT_CMD_XU:   
          fprintf(stderr,"XU\n"); 
          system ("/motor_pwm/IPCAM_motor_Web XU ");
          break;
// ---------------------------------------------- set pan/title any angle command
    case  OUT_CMD_SP:   // set pan angle
         
          //strcat ( tmpcmd , cmd.string ); 
          //fprintf(stderr,tmpcmd); 
          fprintf(stderr,"\nSP\n");

          //system ("/motor_pwm/IPCAM_motor_Web SP&45 "); // syntax ???? how to get parameter ???
          break;
    
    case  OUT_CMD_ST: // set title angle  
          
          //strcat ( tmpcmd , cmd.string ); 
          //fprintf(stderr,tmpcmd); 
          fprintf(stderr,"\nST\n");

          //system ("/motor_pwm/IPCAM_motor_Web SP&45 "); // syntax ???? how to get parameter ???
          break;

  #endif


//joseph add, 2012, Nov 26  ----------------
// for pan-tilt control 
      case OUT_CMD_PAN_SET:
	   fprintf(stderr,"OUT_CMD_PAN_SET\n"); 
           sprintf(cmdbuf,"josephtest  %#x %d",CMD_PAN_SET,value);
          system(cmdbuf); 
	   
	   break;
      case OUT_CMD_TILT_SET:
	   fprintf(stderr,"OUT_CMD_TILT_SET\n"); 
           sprintf(cmdbuf,"josephtest  %#x %d",CMD_TILT_SET,value);
          system(cmdbuf); 
	   break;
      case OUT_CMD_PAN_PLUS:
	   fprintf(stderr,"OUT_CMD_PAN_PLUS\n"); 
           sprintf(cmdbuf,"josephtest %#x",CMD_PAN_PLUS);
           system(cmdbuf); 
	   break;
      case OUT_CMD_PAN_MINUS:
	   fprintf(stderr,"OUT_CMD_PAN_MINUS\n"); 
           sprintf(cmdbuf,"josephtest %#x",CMD_PAN_MINUS);
           system(cmdbuf);
	   break;
      case OUT_CMD_TILT_PLUS:
	   fprintf(stderr,"OUT_CMD_TILT_PLUS\n"); 
           sprintf(cmdbuf,"josephtest %#x",CMD_TILT_PLUS);
           system(cmdbuf);
	   break;
      case OUT_CMD_TILT_MINUS:
	   fprintf(stderr,"OUT_CMD_TILT_MINUS\n"); 
           sprintf(cmdbuf,"josephtest %#x",CMD_TILT_MINUS);
           system(cmdbuf);
	   break;

 
    default:
           fprintf(stderr,"unknow command\n");
          break;
/*
  { "pan_set", OUT_CMD_PAN_SET },
  { "pan_plus", OUT_CMD_PAN_PLUS },
  { "pan_minus", out_CMD_PAN_MINUS },
  { "tilt_set", OUT_CMD_TILT_SET },
  { "tilt_plus", OUT_CMD_TILT_PLUS },
  { "tilt_minus", OUT_CMD_TILT_MINUS },
*/
//------------------------------------------
//jerry 2012-10-09
/*
      case OUT_CMD_CAR_GO_F:
	   fprintf(stderr,"car go f\n"); 
	   system ("/bin/ezctl 1");
	   break;

      case OUT_CMD_CAR_STOP:
	   fprintf(stderr,"car stop\n"); 
	   system ("/bin/ezctl 2");
	   break;

      case OUT_CMD_CAR_GO_R:
	   fprintf(stderr,"car go right\n"); 
	   system ("/bin/ezctl 3");
	   break;
      case OUT_CMD_CAR_GO_L:
	   fprintf(stderr,"car go left\n"); 
	   system ("/bin/ezctl 4");
	   break;
      case OUT_CMD_CAR_GO_B:
	   fprintf(stderr,"car go back\n"); 
	   system ("/bin/ezctl 5");
	   break;
      case OUT_CMD_LED_F_ON:
	   fprintf(stderr,"led_f on \n"); 
	   system ("/bin/led2_test 1 1");
	   break;
      case OUT_CMD_LED_B_ON:
	   fprintf(stderr,"led_b on\n"); 
	   system ("/bin/led2_test 2 1");
	   break;
      case OUT_CMD_LED_F_OFF:
	   fprintf(stderr,"led_f off\n"); 
	   system ("/bin/led2_test 1 2");
	   break;
      case OUT_CMD_LED_B_OFF:
	   fprintf(stderr,"led_b off\n"); 
	   system ("/bin/led2_test 2 2");
	   break;
      case OUT_CMD_MUSIC_ON:
	   fprintf(stderr,"led_b off\n"); 
	   system ("/bin/led2_test 3 1");
	   break;
      case OUT_CMD_MUSIC_OFF:
	   fprintf(stderr,"led_b off\n"); 
	   system ("/bin/led2_test 3 2");
	   break;
*/
  
  }


  return 0;
}
