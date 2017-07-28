CC=g++
CFLAGS=-g -c -std=c++11 -Iannoy/src -Isrc -shared -o build/libannoy.so -fPIC
OBJECTS=src/annoy.cpp

compile: 
	- mkdir build
	$(CC) $(CFLAGS) $(OBJECTS)
