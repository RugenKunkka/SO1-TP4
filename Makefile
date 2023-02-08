CFLAGS = -Wall -Werror -pedantic 

main: main.o
	gcc $(CFLAGS) -o main main.o 

main.o: main.c 
	gcc $(CFLAGS) -c main.c 

clean:
	rm -f *.o *.a main
    
