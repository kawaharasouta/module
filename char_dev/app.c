#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>

#define DEV "/dev/char_dev"

int main() {
	int fd = open(DEV, O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "open err\n");
		return -1;
	}

	close(fd);
	return 0;
}
