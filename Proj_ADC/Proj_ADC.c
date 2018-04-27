/*
 * RTU_K.c
 *
 *  Created on: Nov 26, 2016
 *      Author: dewn2d
 */


#ifndef MODULE
#define MODULE
#endif

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <linux/time.h>


MODULE_LICENSE("GPL");

static RT_TASK tsk1;
RTIME period, period2;

char* High;
short* Low;
unsigned long* install;
unsigned char* Conversion;

static void rt_process(int t)
{
	unsigned short volt = 2300;

	if((*install & 0x1) == 1)
	{

		while(1)
		{

			*High = 0x40; //write 01000000 to 0x10F00000

			while((*Conversion >> 7) == 1)
			{
				rt_sleep(nano2count(100000));//wait .1ms
			}

			volt = *Low & 0xFFF;

			rt_task_make_periodic(&tsk1, rt_get_time(), period);

			rtf_put(3, &volt, sizeof(unsigned short));
			rt_task_wait_period();
		}
	}

}


int init_module(void)
{
	rtf_create(3, sizeof(unsigned short));

	High = (char*) __ioremap(0x10F00000, 4096, 0);
	Low = (short*) __ioremap(0x10F00000, 4096, 0);
	install = (unsigned long*) __ioremap(0x22400000, 4096, 0);
	Conversion = (unsigned char*) __ioremap(0x10800000, 4096, 0);

	rt_set_periodic_mode();//set to periodic mode
	period = start_rt_timer(nano2count(100000000));//.1ms
	rt_task_init(&tsk1, rt_process, 0, 256, 0, 0, 0);
	rt_task_make_periodic(&tsk1, rt_get_time(), period2);

	return 0;
}

void cleanup_module(void)
{

	rt_task_delete(&tsk1);//deletes real time task
	rtf_destroy(3);
	stop_rt_timer();//stops timer

}



