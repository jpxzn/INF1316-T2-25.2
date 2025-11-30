
.PHONY: all run clean

all:
	gcc -o app app.c
	gcc -o kernel kernel.c
	gcc -o interController interController.c

run: all
	./kernel

clean:
	rm -f app kernel interController