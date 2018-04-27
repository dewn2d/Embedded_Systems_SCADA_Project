/*
 * test.c
 *
 *  Created on: Oct 19, 2016
 *      Author: dewn2d
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

unsigned long *PFDR,*PFDDR;
unsigned long *PBDR,*PBDDR, *bRawSts;

int main(void)
{
	int fifo_in, pr, cnt = 0;
	unsigned short volt;
	float volts[100];

	fifo_in = open("/dev/rtf/3", O_RDWR);

	while(1)
	{
		cnt = 0;
		pr = 0;
		printf("Enter 1 to get count\n");
		scanf("%d",&pr);
		if (pr == 1)
		{

			while( cnt < 100)
			{

				read(fifo_in, &volt, sizeof(unsigned short));
				volts[cnt] = (float)(5 * (float)volt) / (float)4095;

				printf(" voltage: %f\n",volts[cnt]);
				cnt++;

			}

		}

	}


return 0;
}
