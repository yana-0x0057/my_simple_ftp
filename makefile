all:	myftps	myftpc
myftps:	myftps.o
	gcc  -o myftps myftps.o
	rm myftps.o
myftps.o:	myftps.c	myls.c	ftph.c	file.c	getargs.c
	gcc -c myftps.c
myftpc:	myftpc.o
	gcc -o myftpc myftpc.o
	rm myftpc.o
myftpc.o:	myftpc.c	myls.c	ftph.c	file.c	getargs.c
	gcc -c myftpc.c
