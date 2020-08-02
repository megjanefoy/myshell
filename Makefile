# this will compile myshell.c file and create an executible called myshell. 
#It also has the option to clean the executable and object files.

all: 
	gcc myshell.c -o myshell
clean: 
	rm -f *.exe *.o *~ myshell