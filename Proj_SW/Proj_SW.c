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
#include <asm/delay.h>

MODULE_LICENSE("GPL");

//global vars
static RT_TASK tsk1;
RTIME period;

unsigned long *PFDR,*PFDDR;
unsigned long *PBDR,*PBDDR;
unsigned long *IntEn, *Type1,*Type2,*EOI ,*RawInt,*DeBo;
unsigned long *VIC2_soft,*VIC2_IntEn,*VIC2_clear;

char input;

typedef struct timeval TIME;

void ISR_HW(unsigned irq_num, void *cookie)
{
	//disable 59
	rt_disable_irq(59);
	int output;


	if((*RawInt & 0x01))// check button 1
	{
//		printk("sent 1");
		output = 1;
		rtf_put(2, &output, sizeof(int)); //send frequency through fifo to RTU
	}
	if((*RawInt & 0x02))
	{
		output = 2;
		rtf_put(2, &output, sizeof(int));
	}
	if((*RawInt & 0x04))
	{
		output = 3;
		rtf_put(2, &output, sizeof(int));
	}
	if((*RawInt & 0x08))
	{
		output = 4;
		rtf_put(2, &output, sizeof(int));
	}

	
	*EOI |= 0x1F;//clear rawIntStsB

	//enable 59
	rt_enable_irq(59);
}

int init_module(void)
{

	unsigned long *ptr;

	ptr = (unsigned long*) __ioremap(0x80840000, 4096, 0);

	PBDDR = ptr + 5;
	PBDR = ptr + 1;
	PFDDR = ptr + 13;
	PFDR = ptr + 12;
    *PBDDR |= 0xE0;
	*PFDDR |= 0x02;

	Type1 = ptr + 43;//GPIOBTYPE2
	Type2 = ptr + 44;//GPIOBTYPE2
	EOI = ptr + 45;//GPIOBEOI
	IntEn = ptr + 46;//GPIOBINTEN
	RawInt = ptr + 48;	//RAWINTSTSB
	DeBo = ptr + 49; //GPIOBDebounce

	*DeBo |= 0x1F;
	*IntEn |= 0x1F;
	*EOI |= 0x1F;
	*Type2 &= 0xE0;
	*Type1 |= 0x1F;

	rtf_create(2, sizeof(int));

	rt_request_irq(59, ISR_HW, 0, 1);
	*EOI |= 0x1F;
	*IntEn |= 0x1F;
	rt_enable_irq(59);

	return 0;

}

void cleanup_module(void)
{

	rt_task_delete(&tsk1);//deletes real time task
	rtf_destroy(2);
	rt_disable_irq(59);
	rt_release_irq(59);
	stop_rt_timer();//stops timer

}


