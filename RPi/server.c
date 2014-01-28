//----- Include files ---------------------------------------------------------
#include <stdio.h>          // for printf()
#include <stdlib.h>         // for exit()
#include <string.h>         // for strcpy(),strerror() and strlen()
#include <fcntl.h>          // for file i/o constants
#include <sys/stat.h>       // for file i/o constants
#include <errno.h>
 
/* FOR BSD UNIX/LINUX  ---------------------------------------------------- */
#include <sys/types.h>      //   
#include <netinet/in.h>     //   
#include <sys/socket.h>     // for socket system calls  
#include <arpa/inet.h>      // for socket system calls (bind) 
#include <sched.h>   
#include <pthread.h>        /* P-thread implementation        */    
#include <signal.h>         /* for signal                     */ 
#include <semaphore.h>      /* for p-thread semaphores        */
#include <sys/time.h>
/* ------------------------------------------------------------------------ */ 

//----- Defines -------------------------------------------------------------
#define BUF_SIZE            1024     // buffer size in bytes
#define PORT_NUM           27000     // Port number for a Web server (TCP 5080)
#define PEND_CONNECTIONS     100     // pending connections to hold 
#define TRUE                   1
#define FALSE                  0
#define NTHREADS 5                     /* Number of child threads        */ 
#define NUM_LOOPS  10                  /* Number of local loops          */
#define SCHED_INTVL 5                  /* thread scheduling interval     */
#define HIGHPRIORITY 10
 
 // Wiring PI
#include <wiringPi.h>
 
 // UART
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART

// SIGTERM
#include <signal.h>
 
/* global variables ---------------------------------------------------- */
sem_t thread_sem[NTHREADS];  
int   next_thread;  
int   can_run;
int   i_stopped[NTHREADS]; 
 
unsigned int    client_s;               // Client socket descriptor
int uart0_filestream ;
 unsigned int          server_s;               // Server socket descriptor
 int serverRun;
 
struct timeval start, end;
  
/* Child thread implementation ----------------------------------------- */
void *my_thread(void * arg)
{
    unsigned int    myClient_s;         //copy socket
     
    /* other local variables ------------------------------------------------ */
  char           in_buf[BUF_SIZE];           // Input buffer for GET resquest
  char           out_buf[BUF_SIZE];          // Output buffer for HTML response
  char           *file_name;                 // File name
  unsigned int   fh;                         // File handle (file descriptor)
  unsigned int   buf_len;                    // Buffer length for file reads
  unsigned int   retcode;                    // Return code
 
  myClient_s = *(unsigned int *)arg;        // copy the socket
 
  /* receive the first HTTP request (HTTP GET) ------- */
  int lastTime = 0;
  gettimeofday(&end, NULL);
  while(serverRun)
  {
      retcode = recv(client_s, in_buf, BUF_SIZE, 0);
 
      /* if receive error --- */
      if (retcode < 0)
      {   printf("recv error detected ...\n"); }
      /* if HTTP command successfully received --- */
      else
      {    
		if ( in_buf[0] == 'X' )
			break;
		
		gettimeofday(&start, NULL);
		long seconds  = start.tv_sec  - end.tv_sec;
		long useconds = start.tv_usec - end.tv_usec;
		long mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

		printf("Elapsed time: %ld milliseconds\n", mtime);
		
		if ( mtime > 250 )
		{
			end.tv_sec = start.tv_sec;
			end.tv_usec = start.tv_usec;
			/* Parse out the filename from the GET request --- */
			// Proceess data in in_buf
			printf("-------------------------\n",in_buf);
			printf("%s\n",in_buf);
			printf("VAL BEFORE:%d %d %d %d \n", retcode, (int)(in_buf[0] -'0'), (int)(in_buf[1]-'0'), (int)(in_buf[2] -'0'));
			int val = (int)(in_buf[1]-'0') * 10 + (int)(in_buf[2] -'0');
			in_buf[0] = (unsigned char) val;
			in_buf[1] = '\0';
			printf("SPI sending: '%d', val: %d \n", (int)in_buf[0], val);
			wiringPiSPIDataRW(0,in_buf,1);
			printf("SPI recv: '%d'\n", (int)in_buf[0]);
			
			in_buf[0] = (unsigned char) val;
			if (uart0_filestream != -1)
			{
				printf("UART sending '%d'\n", (int)in_buf[0]);
				int count = write(uart0_filestream, &in_buf[0], 1);		//Filestream, bytes to write, number of bytes to write
				if (count < 0)
				{
					printf("UART TX error\n");
				}
			}
			
			
			if (uart0_filestream != -1)
			{
				// Read up to 255 characters from the port if they are there
				unsigned char rx_buffer[256];
				int rx_length = read(uart0_filestream, (void*)rx_buffer, 255);		//Filestream, buffer to store in, number of bytes to read (max)
				if (rx_length < 0)
				{
					//An error occured (will occur if there are no bytes)
				}
				else if (rx_length == 0)
				{
					printf("UART: nothing to read\n");
					//No data waiting
				}
				else
				{
					//Bytes received
					printf("UART: %i bytes read : %d %d \n", rx_length, (int)rx_buffer[0]);
				}
			}
		}
		
		
      }
  }
  close(client_s); // close the client connection
  pthread_exit(NULL);
}
 
 
 void term(int signum)
{
    printf("Received SIGTERM, exiting...\n");
    serverRun = 0;
	printf("Server run = %d\n",serverRun);
	//close (server_s);  // close the primary socket
}

//===== Main program ========================================================
int main(void)
{
	struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGINT, &action, NULL);
	serverRun = 1;
	
	if(wiringPiSPISetup(0,1000000) <0 )
	{
		fprintf(stderr,"Unable to open SPI device 0: %s\n", strerror(errno));
		exit(1);
	}
	printf("Successful setup of SPI!\n");
	
	
	uart0_filestream = -1;
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}
	
	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);


  /* local variables for socket connection -------------------------------- */
  struct sockaddr_in    server_addr;            // Server Internet address
  //unsigned int            client_s;           // Client socket descriptor
  struct sockaddr_in    client_addr;            // Client Internet address
  struct in_addr        client_ip_addr;         // Client IP address
  int                   addr_len;               // Internet address length
 
  unsigned int          ids;                    // holds thread args
  pthread_attr_t        attr;                   //  pthread attributes
  pthread_t             threads;                // Thread ID (used by OS)
 
  /* create a new socket -------------------------------------------------- */
  server_s = socket(AF_INET, SOCK_STREAM, 0);
 
  /* fill-in address information, and then bind it ------------------------ */
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT_NUM);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
 
  /* Listen for connections and then accept ------------------------------- */
  listen(server_s, PEND_CONNECTIONS);
 
  /* the web server main loop ============================================= */
  pthread_attr_init(&attr);
  while(serverRun)
  {
    printf("my server is ready ...\n");  
 
    /* wait for the next client to arrive -------------- */
    addr_len = sizeof(client_addr);
    client_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len);
	if (serverRun == 0 )
		break;
	//printf("Server run = %d\n",serverRun);
 
    printf("a new client arrives ...\n");  
 
    if (client_s == FALSE)
    {
      printf("ERROR - Unable to create socket \n");
      exit(FALSE);
    }
 
    else
    {
        /* Create a child thread --------------------------------------- */
        ids = client_s;
        pthread_create (                    /* Create a child thread        */
                   &threads,                /* Thread ID (system assigned)  */    
                   &attr,                   /* Default thread attributes    */
                   my_thread,               /* Thread routine               */
                   &ids);                   /* Arguments to be passed       */
 
    }
  }
 
  /* To make sure this "main" returns an integer --- */
  close (server_s);  // close the primary socket
  return (TRUE);        // return code from "main"
}
