
.PHONY: all run clean server

all:
	gcc -o app app.c
	gcc -o kernel kernel.c
	gcc -o interController interController.c

run: all
	./kernel

server:
	gcc -o server SFSS.c
	./server

clean:
	rm -f app kernel interController server