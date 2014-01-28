#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
int main(void)
{
	if(wiringPiSPISetup(0,8000000) <0 )
	{
		fprintf(stderr,"Unable to open SPI device 0: %s\n", strerror(errno));
	exit(1);
	}
	printf("Successful setup!\n");

	unsigned char buff[10];
	buff[0] = 'A';
	buff[1] = 'B';
	buff[2] = 'C';
	wiringPiSPIDataRW(0,buff,10);
	printf("%s\n",buff);


}
