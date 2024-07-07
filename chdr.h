#ifndef CHDR_H
#define CHDR_H

#include <linux/ioctl.h>
#include <linux/sched.h> //TASK_COMM_LEN


/* Use 'k' as magic number */
#define BP_IOC_MAGIC  'k'

#define BP_IOC_GETWRITERS_TIME	_IOR(BP_IOC_MAGIC,  0, u64)
#define BP_IOC_GETWRITERS_PID	_IOR(BP_IOC_MAGIC,  1, int)
#define BP_IOC_GETWRITERS_COMM	_IOR(BP_IOC_MAGIC,  2, char *)

#define BP_IOC_GETREADERS_TIME	_IOR(BP_IOC_MAGIC,  3, u64)
#define BP_IOC_GETREADERS_PID	_IOR(BP_IOC_MAGIC,  4, int)
#define BP_IOC_GETREADERS_COMM	_IOR(BP_IOC_MAGIC,  5, char *)

struct bp_user{
	u64 time; //ktime_t ktime_get_real(void)
	int pid;
	char comm[TASK_COMM_LEN]; //TASK_COMM_LEN = 16 (linux/sched.h)
};


#endif /*CHDR_H*/