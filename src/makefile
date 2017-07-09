CC = gcc

PROG = jms_coord jms_console pool
	
all : $(PROG)

jms_coord: jms_coord.o functions.o
	$(CC) $^ -o $@ functions.h
	
jms_console: jms_console.o 
	$(CC) $^ -o $@ 
	
pool: pool.o functions.o
	$(CC) $^ -o $@ functions.h	
	
clean :
	rm -f jms_coord.o jms_console.o functions.o pool.o
	