#Makefile
build: jobshop.c simlib.c
	gcc jobshop.c simlib.c -lm -o jobshop

run: 
	./jobshop

build-run: build run