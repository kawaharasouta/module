.PHONY: all clean install login app

obj-m := char_dev.o

all:
	make -C ../../../output M=$(PWD) modules

clean:
	make -C ../../../linux M=$(PWD) clean

install:
	cp *.ko ../../../elkdat; cd ../../../elkdat; vagrant rsync

login:
	cd ../../../elkdat; vagrant ssh

app:
	gcc app.c -o app.out
