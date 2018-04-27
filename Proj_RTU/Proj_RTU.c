
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <semaphore.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include <signal.h>

#define MSG_SIZE 100
#define EVNT_SIZE 10000


//holds event information
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

void switch1(void* ptr);
void adc1(void* ptr);
void exit1(void* ptr);
void printlog(int a);
double getstmp(struct timeval now);
//void child( int sox, struct sockaddr_in server, socklen_t serverlength, struct timeval start);

char buffer[MSG_SIZE];

struct timeval start;

sem_t sem;
sem_t sem2;

int ex=1;

log log1;
log logArray[EVNT_SIZE];

int evntnum = 0;
int dc_evntnum = 0;

int cyc = 0;

int num = 0;
int but1 = 0;
int but2 = 0;
int but3 = 0;
int but4 = 0;

float volts = 0;
float sent_volts = 0;
int cnt = 0;

int flag = 0;

RT_TASK *rt_process;
RTIME period;

int main (int argc, char *argv[])
{

	pthread_t thrd1, thrd2, thrd3;

	if (sem_init(&sem,0, 1) == -1)
	{
		printf("Error creating Semaphore");
		exit(2);
	}

	if (sem_init(&sem2,0, 1) == -1)
	{
		printf("Error creating Semaphore");
		exit(2);
	}

	int sox, length, n;//, flag = 0
	char addr[MSG_SIZE], ip[MSG_SIZE] , serveraddr[MSG_SIZE];
	char boardnum[MSG_SIZE];
	struct hostent *serv;
	int chk = 0;

	printf("Enter Historian board #: ");
	scanf("%d", &num);

	gethostname(ip, sizeof(ip));
	struct hostent *ipADDR;
	struct in_addr **boardIPlist;
	ipADDR = (struct hostent *) gethostbyname(ip);
	boardIPlist = (struct in_addr **) ipADDR -> h_addr_list;
	strcpy(addr, inet_ntoa(*boardIPlist[0]));

	//unsigned long convaddr = nam2num(serveraddr);

	char tmp[MSG_SIZE];
	strcpy( tmp, addr);

	char* addrtoken[4];
	addrtoken[0] = strtok(tmp, ".");
	addrtoken[1] = strtok(NULL, ".");
	addrtoken[2] = strtok(NULL, ".");
	addrtoken[3] = strtok(NULL, ".");
	sprintf(boardnum, "%d", num);
	printf("Board IP: %s\n", addr);

	strcpy(serveraddr, addrtoken[0]); // construct the string containing the client address
	strcat(serveraddr, ".");
	strcat(serveraddr, addrtoken[1]);
	strcat(serveraddr, ".");
	strcat(serveraddr, addrtoken[2]);
	strcat(serveraddr, ".");
	strcat(serveraddr, boardnum);

	printf("connecting to board %s through port %s\n",serveraddr,argv[1]);
//	printf("size of addr %d\n",strlen(serveraddr));
	struct sockaddr_in server; //from;
	//struct in_addr sox_in_ad;

	log1.rtu = atoi(addrtoken[3]);

	sox = socket(AF_INET, SOCK_STREAM, 0); // socket file descriptor

	length = sizeof(server);			// length of structure

	serv = gethostbyname(serveraddr);
	if (serv == NULL)
	{
		printf("ERROR, no such host\n");
		exit(2);
	}
	bzero(&server,length);

	server.sin_port = htons(atoi(argv[1]));
	bcopy((char *)serv->h_addr, (char *)&server.sin_addr.s_addr, serv->h_length);
	server.sin_family = AF_INET;

	//	bind( sox, (struct sockaddr *) &server, length); // bind socket

	socklen_t serverlength = sizeof(struct sockaddr_in);
	if(connect(sox,(struct sockaddr *) &server, serverlength) < 0)
	{
		printf("Not connected\n");
		close(sox);
		exit(2);
	}

	struct timeval now;

	pthread_create(&thrd3, NULL, (void *)&exit1, argv[1]);
	pthread_create(&thrd1, NULL, (void *)&switch1, NULL);
	pthread_create(&thrd2, NULL, (void *)&adc1, NULL);

	while (ex != 0)
	{


		if(flag == 0)
		{

			sem_wait(&sem);

			gettimeofday(&now, NULL);

			log1.stmp = getstmp(now);
			log1.volts = volts;
			if ( volts == sent_volts)
			{
				cnt++;
			}

			else if ( volts != sent_volts)
			{
				cnt = 0;
			}

			sent_volts = log1.volts;
			log1.act_events += evntnum;
			log1.num_events = evntnum;
			log1.S1 = but1;
			log1.S2 = but2;
			log1.S3 = but3;
			log1.S4 = but4;

			n = sendto(sox, &log1, sizeof(log),0,(const struct sockaddr *) &server, serverlength);// send message
			if(n < 0 && chk == 0)
			{
				if(connect(sox,(struct sockaddr *) &server, serverlength) < 0)
				{
					printf("Error connecting to socket\n Entering offline mode...\n");
					chk++;
				}
			}

			else if(n < 0 && chk == 1)
			{
				if(connect(sox,(struct sockaddr *) &server, serverlength) < 0)
				{
					printf("log saved");
					logArray[dc_evntnum] = log1;
					dc_evntnum++;
				}
			}

			else if(n == sizeof(log) && chk == 1)
			{
				chk =0;

				while ( dc_evntnum > 0)
				{

					printf("Sending Lost Data...\n");
					n = sendto(sox, &log1, sizeof(log),0,(const struct sockaddr *) &server, serverlength);
					if (n < 0)
					{
						printf("Error writing socket 2\n");
						exit(2);
					}

					dc_evntnum--;
					rt_task_wait_period();

				}

			}

			else
			{

//				printlog(1);

				while (evntnum > 0)
				{
					bzero(log1.events[evntnum].evnt_type,1000);
					evntnum--;
				}

			}

			flag = 1;

		}

		else if (flag == 1)
		{
			sem_wait(&sem2);

			gettimeofday(&now, NULL);

			log1.stmp = getstmp(now);
			log1.volts = volts;
			if ( volts == sent_volts)
			{
				cnt++;
			}

			else if ( volts != sent_volts)
			{
				cnt = 0;
			}

			sent_volts = log1.volts;
			log1.act_events += evntnum;
			log1.num_events = evntnum;
			log1.S1 = but1;
			log1.S2 = but2;
			log1.S3 = but3;
			log1.S4 = but4;

			n = sendto(sox, &log1, sizeof(log),0,(const struct sockaddr *) &server, serverlength);// send message
			if(n < 0 && chk == 0)
			{
				if(connect(sox,(struct sockaddr *) &server, serverlength) < 0)
				{
					printf("Error connecting to socket\n Entering offline mode...\n");
					chk++;
				}
			}

			else if(n < 0 && chk == 1)
			{
				if(connect(sox,(struct sockaddr *) &server, serverlength) < 0)
				{
					printf("log saved");
					logArray[dc_evntnum] = log1;
					dc_evntnum++;
				}
			}

			else if(n == sizeof(log) && chk == 1)
			{
				chk =0;

				while ( dc_evntnum > 0)
				{

					printf("Sending Lost Data...\n");
					n = sendto(sox, &log1, sizeof(log),0,(const struct sockaddr *) &server, serverlength);
					if (n < 0)
					{
						printf("Error writing socket 2\n");
						exit(2);
					}

					dc_evntnum--;
					rt_task_wait_period();

				}

			}

			else
			{

//				printlog(2);

				while (evntnum > 0)
				{

					bzero(log1.events[evntnum].evnt_type,1000);
					evntnum--;

				}

			}

			flag = 0;

		}

		usleep(1000000);

			sem_post(&sem);
			sem_post(&sem2);

	}

	close(sox);

	return 0;
}

void switch1(void* ptr)
{


	int fifo_in, swch;
	struct timeval now;
	fifo_in = open("/dev/rtf/2", O_RDWR);//open fifo for reading

	while(ex != 0)
	{

		read(fifo_in, &swch, sizeof(swch));


		if (flag == 0)
		{
			sem_wait(&sem);
			flag = 1;
			gettimeofday(&now, NULL);
			log1.events[evntnum].stmp = getstmp(now);
			if ( swch == 1 && but1 == 0)
			{
				but1 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 1 && but1 == 1)
			{
				but1 = 0;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 2 && but2 == 0)
			{
				but2 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 2 && but2 == 1)
			{
				but2 = 0;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 3 && but3 == 0)
			{
				but3 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 3 && but3 == 1)
			{
				but3 = 0;			rt_task_wait_period();
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 4 && but4 == 0)
			{
				but4 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 4 && but4 == 1)
			{
				but4 = 0;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off ", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			evntnum++;
			sem_post(&sem);
		}

		if (flag == 1)
		{
			sem_wait(&sem2);
			gettimeofday(&now, NULL);
			log1.events[evntnum].stmp = getstmp(now);
			if ( swch == 1 && but1 == 0)
			{
				but1 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 1 && but1 == 1)
			{
				but1 = 0;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 2 && but2 == 0)
			{
				but2 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 2 && but2 == 1)
			{
				but2 = 0;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 3 && but3 == 0)
			{
				but3 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 3 && but3 == 1)
			{
				but3 = 0;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 4 && but4 == 0)
			{
				but4 = 1;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned on", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			else if ( swch == 4 && but4 == 1)
			{
				but4 = 0;
				sprintf(log1.events[evntnum].evnt_type, "Switch%u: turned off", swch);
				//			printf("%s",log1.events[evntnum].evnt_type);

			}

			evntnum++;
			sem_post(&sem2);
		}

	}
	pthread_exit(0);
}

void adc1(void* ptr)
{

	int fifo_in;
	int noP = 2;
	float overld = 4, Low = 1.5;
	unsigned short volt;
	struct timeval now;

	fifo_in = open("/dev/rtf/3", O_RDWR);

	while(ex != 0)
	{

		if (read(fifo_in, &volt, sizeof(unsigned short)) != sizeof(unsigned short))
		{
			printf("Error with ADC fifo\n");
			exit(2);
		}

		gettimeofday(&now, NULL);


		volts = (float)(5 * (float)volt) / (float)4095;


		if (cnt >= noP)
		{

			if (flag == 0)
			{

				sem_wait(&sem);
				log1.events[evntnum].stmp = getstmp(now);
				sprintf(log1.events[evntnum].evnt_type, "No Power");
				evntnum++;
				sem_post(&sem);

			}

			if (flag == 1)
			{

				sem_wait(&sem2);
				log1.events[evntnum].stmp = getstmp(now);
				sprintf(log1.events[evntnum].evnt_type, "No Power");
				evntnum++;
				sem_post(&sem2);

			}

			while(cnt >= noP )
			{
				if (read(fifo_in, &volt, sizeof(unsigned short)) != sizeof(unsigned short))
				{
					printf("Error with ADC fifo\n");
					exit(2);
				}

				gettimeofday(&now, NULL);

				volts = (float)(5 * (float)volt) / (float)4095;

				if (flag == 0)
				{
					if (cnt >= noP)
					{
						sem_wait(&sem);
						log1.events[evntnum].stmp = getstmp(now);
						sprintf(log1.events[evntnum].evnt_type, "Still No Power");
						evntnum++;
						sem_post(&sem);
					}
				}

				else if (flag == 1)
				{
					if (cnt >= noP)
					{
						sem_wait(&sem2);
						log1.events[evntnum].stmp = getstmp(now);
						sprintf(log1.events[evntnum].evnt_type, "Still No Power");
						evntnum++;
						sem_post(&sem2);
					}
				}

			}
		}

		else if (volts > overld)
		{
			if (flag == 0)
			{
				sem_wait(&sem);
				log1.events[evntnum].stmp = getstmp(now);
				sprintf(log1.events[evntnum].evnt_type, "Line overload: V > %f", overld);
				evntnum++;
				sem_post(&sem);
			}

			if (flag == 1)
			{
				sem_wait(&sem2);
				log1.events[evntnum].stmp = getstmp(now);
				sprintf(log1.events[evntnum].evnt_type, "Line overload: V > %f", overld);
				evntnum++;
				sem_post(&sem2);
			}

		}

		else if (volts < Low)
		{
			if (flag == 0)
			{
				sem_wait(&sem);
				log1.events[evntnum].stmp = getstmp(now);
				sprintf(log1.events[evntnum].evnt_type, "Line below normal operation: V < %f", Low);
				evntnum++;
				sem_post(&sem);
			}

			if (flag == 1)
			{
				sem_wait(&sem2);
				log1.events[evntnum].stmp = getstmp(now);
				sprintf(log1.events[evntnum].evnt_type, "Line below normal operation: V < %f", Low);
				evntnum++;
				sem_post(&sem2);
			}

		}


	}

	pthread_exit(0);

}


void exit1(void* ptr)
{

	while (ex != 0)
	{
		if(num != 0)
		{
			printf("Enter 0 to exit\n");
			scanf("%d",&ex);
		}

		else if (num != 0 && ex != 0)
		{
			ex =20;
			printf("Enter 0 to exit\n");
			scanf("%d",&ex);
		}
	}


}


void printlog(int a)
{
	int i;

	printf("(log %d) %d, %lf, %f, %d %d\n", a,log1.rtu,log1.stmp, log1.volts, log1.act_events, log1.num_events);

	for (i = 0; i < evntnum; i++)
	{
		//		printf(" %ld.%06ld %ld.%06ld\n"log1.events[i].end.tv_sec, log1.events[i].end.tv_usec, log1.events[i].start.tv_sec , log1.events[i].start.tv_usec);

		printf("(log %d) %lf %s\n", a, log1.events[i].stmp,log1.events[i].evnt_type);
	}

	pthread_exit(0);

}

double getstmp(struct timeval now)
{

	double stmp =0;

	stmp = (double) (now.tv_usec) ;
	stmp /= 1000000;
	stmp += (double)now.tv_sec;

	return stmp;

}
