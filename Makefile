# choose your compiler
CC=gcc -Wall

mysh: sh.o get_path.o UserNode.o main.c
	$(CC) -g main.c sh.o UserNode.o get_path.o -o mysh
#	$(CC) -g main.c sh.o get_path.o bash_getcwd.o -o mysh

sh.o: sh.c sh.h
	$(CC) -g -c sh.c

UserNode.o: UserNode.c
	$(CC) -g -c UserNode.c


get_path.o: get_path.c get_path.h
	$(CC) -g -c get_path.c

clean:
	rm -rf *.o mysh *.gch *.stackdump
