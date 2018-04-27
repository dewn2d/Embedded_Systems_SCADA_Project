/*
 * RTU_H.c
 *
 *  Created on: Nov 26, 2016
 *      Author: dewn2d
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <signal.h>

#define MSG_SIZE 100
#define LOG_SIZE 10000

int buffLoc1;
sem_t sem;

struct event
{
	double stmp;
	char evnt_type[MSG_SIZE];
};

//struct that holds information for log file
typedef struct
{
	int rtu;
	double stmp;
	float volts;
	struct event events[50];
	int S1;
	int S2;
	int S3;
	int S4;
	int num_events;
	int act_events;
}log;

int pr = 3;

void recvLog(int sox);
void exit1(void* ptr);
void print();
const char* On_off(int i);


int main (int argc, char *argv[])
{

	pthread_t thrd;

	FILE *fp = fopen("log.txt", "w");
	fclose(fp);
	FILE *fp2 = fopen("events_log.txt", "w");
	fclose(fp2);

	pthread_create(&thrd, NULL, (void *)&exit1, NULL);

	int sox,sox2, length,i=0,pid;//, flag = 0

	struct sockaddr_in server, client; //from;
	//struct in_addr sox_in_ad;

	sox = socket(AF_INET, SOCK_STREAM, 0); // socket file descriptor

	length = sizeof(server);			// length of structure
	bzero(&server,length);

	server.sin_port = htons(atoi(argv[1]));
	//	printf("%d\n", server.sin_port);
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_family = AF_INET;

	bind( sox, (struct sockaddr *) &server, length); // bind socket

	listen(sox, 5);
	socklen_t clientlength = sizeof(struct sockaddr_in);

	while (pr != 0)
	{
		sox2 = accept(sox, (struct sockaddr *) &client, &clientlength);

		i++;
		pid = fork();

		if(sox2 < 0)
		{
			printf("error on accept\n");
			exit(2);
		}

		if (pid == 0)
		{
			printf("Connection #%d created\n",i);
			close(sox);
			recvLog(sox2);
			exit(0);
		}

		if (pr == 0)
		{
			break;
		}

		else
		{
			close(sox2);
			signal(SIGCHLD,SIG_IGN);	// to avoid zombie problem
		}

	}

	close(sox);
	return 0;
}

void recvLog(int sox)
{
	int i=0;

	log buffer;
	buffer.rtu = 0;
	buffer.stmp = 0;

	while( pr != 0)
	{

		if(recvfrom(sox, &buffer, sizeof(log), MSG_WAITALL, NULL, NULL) == sizeof(log))// recvfrom() could be used
		{

			FILE *fp = fopen("log.txt", "a");//open file in append mode
			fprintf(fp, "%lf: RTU %d, Voltage at line %f, Events %d, Switches %s %s %s %s\n", buffer.stmp, buffer.rtu, buffer.volts, buffer.act_events,On_off(buffer.S1),On_off(buffer.S2),On_off(buffer.S3),On_off(buffer.S4)); //write to file
			fclose(fp);
			//append each event to the end of the line
			FILE *fp2 = fopen("events_log.txt", "a");//open file in append mode
			for(i=0; i<buffer.num_events; i++)
			{
				fprintf(fp2, "%lf: RTU %d \"%s\"\n", buffer.events[i].stmp, buffer.rtu, buffer.events[i].evnt_type);
			}

			fclose(fp2);

		}

		else
		{
			close(sox);
		}
	}

	close(sox);

}

void exit1(void* ptr)
{


	while(pr != 0)
	{
		if (pr != 0 || pr != 1 || pr != 2)
		{
			printf("Enter 0 to exit, 1 to print log or 2 to clear log.\n");
			scanf("%d", &pr);
		}

		if (pr == 1)
		{
			print();

			pr = 3;
		}

		else if (pr == 2)
		{
			FILE *fp = fopen("log.txt", "w");
			fclose(fp);
			FILE *fp2 = fopen("events_log.txt", "w");
			fclose(fp2);
			printf("Log cleared.\n");
		}

			usleep(1000000);

	}
	pthread_exit(0);
}

void print( )
{

	char tmp[LOG_SIZE];

	int i=0, j,k;

	char buffer[MSG_SIZE];

	char bufferArray[LOG_SIZE][MSG_SIZE];

	double stmp1 = 1;
	double stmp2 = 1;


	FILE *fp = fopen("log.txt", "r");

	while(fgets(tmp, sizeof(tmp), fp) != NULL)
	{

		sprintf(bufferArray[i], "%s", tmp);

		if (i >= LOG_SIZE)
		{
			printf("Error to many elements");
			fclose(fp);
			pr = 1;
			usleep(1000000);
		}

		i++;

	}

	fclose(fp);


	int rtu;
	int evntnum=0;
	float volt;
	char S1[4];
	char S2[4];
	char S3[4];
	char S4[4];


	for(j=0; j < i; j++)
	{
		k = j;

		sscanf(bufferArray[k],"%lf: RTU %d, Voltage at line %f, Events %d, Switches %s %s %s %s\n", &stmp1, &rtu, &volt, &evntnum,S1,S2,S3,S4);
		sscanf(bufferArray[k-1],"%lf: RTU %d, Voltage at line %f, Events %d, Switches %s %s %s %s\n", &stmp2, &rtu, &volt, &evntnum,S1,S2,S3,S4);

		while(k > 0 && stmp1 < stmp2)
		{
			strcpy(buffer, bufferArray[k]);
			strcpy(bufferArray[k], bufferArray[k-1]);
			strcpy(bufferArray[k-1], buffer);
			k--;
		}

	}
	char tmp2[LOG_SIZE];

	int i2=0, j2,k2;

	char buffer2[MSG_SIZE];
	char bufferArray2[LOG_SIZE][MSG_SIZE];

	static double allstmp[LOG_SIZE];
	static double allstmp2[LOG_SIZE];

	FILE *fp2 = fopen("events_log.txt", "r");

	while(fgets(tmp2, sizeof(tmp2), fp2) != NULL)
	{
		sprintf(bufferArray2[i2], "%s", tmp2);

		if (i2 >= LOG_SIZE)
		{
			printf("Error to many elements");
			fclose(fp2);
			pr = 1;
			usleep(1000000);
		}

		i2++;

	}

	fclose(fp2);

//	int rtu;
	char events[MSG_SIZE];

	for(j2=0; j2 < i2; j2++)
	{
		k2 = j2;

		sscanf(bufferArray2[k2],"%lf: RTU %d \"%s\"\n", &stmp1, &rtu, events);
		sscanf(bufferArray2[k2-1],"%lf: RTU %d \"%s\"\n", &stmp2, &rtu, events);

		while(k2 > 0 && stmp1 < stmp2)
		{
			strcpy(buffer2, bufferArray2[k2]);
			strcpy(bufferArray2[k2], bufferArray2[k2-1]);
			strcpy(bufferArray2[k2-1], buffer2);
			k2--;
		}

	}

	for(j2=0; j2 < i2; j2++)
	{

		sscanf(bufferArray2[j2],"%lf: RTU %d \"%s\"\n", &allstmp2[j2], &rtu, events);

	}

	for(j=0; j < i; j++)
	{
		sscanf(bufferArray[j],"%lf: RTU %d, Voltage at line %f, Events %d\n", &allstmp[j], &rtu, &volt, &evntnum);

	}

	j= j2 = 0;
	  while (j < i && j2 < i2)
	  {

		 if(allstmp[j] == i)
		      printf("%s", bufferArray[j]);

		 else if(allstmp2[j2] == i2)
		      printf("%s", bufferArray[j2]);

		 else if (allstmp[j] < allstmp2[j2])
	    {
	      printf("%s", bufferArray[j]);
	      j++;
	    }

	    else
	    {
	      printf("%s", bufferArray2[j2]);
	      j2++;
	    }

	  }

	  /* Print remaining elements of the larger array */
	  while(j < i)
	  {
	   printf("%s", bufferArray[j]);
	   j++;
	  }
	  while(j2 < i2)
	  {
	   printf("%s", bufferArray2[j2]);
	   j2++;
	  }


}

const char* On_off(int i)
{
	if (i == 1)
		return "On";

	else if (i== 0)
		return "off";

	else
		return "N/A";

}
