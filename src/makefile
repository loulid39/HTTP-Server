
.PHONY: all test
all: server test

test : server
	./server 2143 5 300 & ./test.sh &


server: txt.c 
	gcc -o server txt.c -pthread 


