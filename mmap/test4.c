#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>

#define SHARE_MEM_SIZE 65535
#define DEVICE_FILENAME "/dev/cyx_mmap"
#define SHARE_MEM_PAGE_COUNT 16

int main(int argc,char* argv[]){
	int dev;
	int loop;
	char* ptrdata;

	dev = open(DEVICE_FILENAME,O_RDWR|O_NDELAY);
	ptrdata = (char*)mmap(0,SHARE_MEM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,dev,0);
	printf("mmap success\n");
	sprintf(ptrdata,"wo shi xiao xiang ge\n");
	printf("mmap:%s\n",ptrdata);
	munmap(ptrdata,SHARE_MEM_SIZE);
	close(dev);
	return 0;
}
