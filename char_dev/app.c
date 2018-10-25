#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#define DEV "/dev/char_dev"

int main() {
	int fd = open(DEV, O_RDWR);
	int ret;
	if (fd < 0) {
		fprintf(stderr, "open err\n");
		return -1;
	}

	printf("write\n");
	int var = 1;
	ret = write(fd, &var, 1);
	if (ret < 0) {
		fprintf(stderr, "write err\n");
		return -1;
	}

	int val;
	ret = read(fd, &val, 1);
	if (ret < 0) {
		fprintf(stderr, "read err\n");
		return -1;
	}
	printf("read val:%d\n", val);

	close(fd);
	return 0;
}
