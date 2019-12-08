
PROGS=join_images peak_count

CFLAGS=-Wall 

all: ${PROGS}

join_images: join_images.o
	gcc -o $@ -l netpbm -lm $@.o
	
peak_count: peak_count.o
	gcc -o $@ -l netpbm $@.o
	
%.o: %.c
	gcc -c ${CFLAGS} $<

push:
	git add .
	git commit -m "update"
	git push


	

