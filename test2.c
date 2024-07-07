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

int main(){
 
	pid_t pid;
	int fd;
	printf ( "-----------------TEST_2_NON_BLOCK----------------------\n");
	pid = fork();

	fd = open(DEVICE_FILE_NAME, O_RDWR);
	int f =1;
	if (ioctl(fd, FIONBIO, &f) == -1)
	{
		printf ( "[%d] error! ioctl() FIONBIO, errno: %d\n", getpid(), errno);
		return -1;
	}
	printf ( "[%d] ioctl() FIONBIO: successful set to 1\n", getpid());
	
	
	if(!pid){
		char buf[128];
		int ret;
		
		errno = 0;
		ret = read(fd, &buf, 128);
		if( ret == 128){
			printf( "[%d] READ: %s\n", getpid(), buf);
		}
		else if(ret == -1)
		{
			if( errno == EAGAIN){
				printf( "[%d] READ: EAGAIN. TEST OK!\n", getpid());
			} else{
				printf( "[%d] READ: UNDEFINED ERROR. TEST NOT OK!\n", getpid());
			}
		}
	}
	if(pid){
		char *long_message = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n";
		int ret, count = 80;
		
		errno = 0;
		sleep(1);
		ret = write(fd, long_message, count);
		printf ( "[%d] WRITE: count= %d, errno= %d\n", getpid(), ret, errno);
		if(ret && (ret < count))
			printf ( "[%d] WRITE: TEST OK!\n", getpid());
	
		return 0;
	}
	
	wait(NULL);
	
	return 0;
}