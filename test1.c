#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <sys/wait.h>

#define DEVICE_FILE_NAME "/dev/chdr" 

#define BP_IOC_MAGIC  'k'

#define BP_IOC_GETWRITERS_TIME	_IOR(BP_IOC_MAGIC,  0, uint64_t)
#define BP_IOC_GETWRITERS_PID	_IOR(BP_IOC_MAGIC,  1, int)
#define BP_IOC_GETWRITERS_COMM	_IOR(BP_IOC_MAGIC,  2, char *)

#define BP_IOC_GETREADERS_TIME	_IOR(BP_IOC_MAGIC,  3, uint64_t)
#define BP_IOC_GETREADERS_PID	_IOR(BP_IOC_MAGIC,  4, int)
#define BP_IOC_GETREADERS_COMM	_IOR(BP_IOC_MAGIC,  5, char *)

int main(){
	
	pid_t pid;
	int fd;
	
	pid = fork();

	fd = open(DEVICE_FILE_NAME, O_RDWR);
	
	if(!pid){
		char buf[15];
		
		int ret = 0,
			retval = 0,
			count = 15;
		
		printf ( "-----------------TEST_1_READ_WRITE----------------------\n");
		while((retval >= 0) && (ret < count)){
			retval = read(fd, buf + ret, count);
			ret += retval;
		}
		printf ( "[%d] READ: %s(%d char)\n", getpid(), buf, ret);
	}
	if(pid){
		const char* message = "simple message\n";
		int retval = 0,
			ret = 0,
			count = 15;
		
		sleep(1);
		while((retval >= 0) && (ret < count)){
			retval = write(fd, message + ret, count);
			ret += retval;
		}
		printf ( "[%d] WRITE: %s(%d char), errno: %d\n", getpid(), message, ret, errno);
		return 0;
	}
	
	wait(NULL);
	
	uint64_t time; //ktime_t ktime_get_real(void)
	char comm[16];
	
	printf ( "-----------------TEST_1_IOCTL----------------------\n");
	ioctl(fd, BP_IOC_GETWRITERS_TIME, &time);
	ioctl(fd, BP_IOC_GETWRITERS_PID, &pid);
	ioctl(fd, BP_IOC_GETWRITERS_COMM, &comm);
	printf ( "WRITERS pid: %d\n", pid );
	printf ( "WRITERS time: %lu\n", time);
	printf ( "WRITERS comm: %s\n", comm );
	printf ( "-----------------\n");
	ioctl(fd, BP_IOC_GETREADERS_TIME, &time);
	ioctl(fd, BP_IOC_GETREADERS_PID, &pid);
	ioctl(fd, BP_IOC_GETREADERS_COMM, &comm);
	printf ( "READERS pid: %d\n", pid );
	printf ( "READERS time: %lu\n", time);
	printf ( "READERS comm: %s\n", comm );
	
	return 0;
	
}
