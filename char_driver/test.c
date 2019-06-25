#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

int main(int argc,char* argv[]){
	int fd;
	fd = open("/dev/hello",O_RDWR);
	if(fd < 0){
		printf("open file \"/dev/hello\" failed\n");
		exit(-1);
	}

	printf("please write something:\n");
	char write_buf[2048];
	scanf("%s",write_buf);
	int n = write(fd,write_buf,strlen(write_buf));
	printf("write len = %d : %s\n",n,write_buf);
	llseek(fd,0,0);
	char read_buf[2048];
	n = read(fd,read_buf,n);
	printf("follow is read:\n");
	printf("%s\n",read_buf);
	close(fd);
	return 0;
}
