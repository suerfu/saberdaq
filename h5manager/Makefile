# Makefile for compiing saber library for data acquisition with polaris

CC = g++ 

CPP_FILES = $(wildcard src/*.cpp) 
OBJ_FILES = $(patsubst %.cpp, %.o, $(CPP_FILES))

CFLAGS = -Wall -std=c++0x -fPIC -I./include -I/usr/local/include

# HDF5 flags
CFLAGS += -I/usr/local/hdf5/include
LDFLAGS += -L/usr/local/hdf5/lib -lhdf5 -lhdf5_hl -lhdf5_cpp

all : demo

demo : test/demo.o $(OBJ_FILES)
	$(CC) -o $@ $^ ${LDFLAGS}

test/demo.o : test/demo.cpp
	@echo "compiling $@"
	@$(CC) $(CFLAGS) -c $^ -o $@

%.o : %.cpp
	@echo "compiling $@"
	@$(CC) $(CFLAGS) -c $^ -o $@

clean:
	@echo "cleaning..."
	@-rm ${OBJ_FILES} > /dev/null 2>&1
	@-rm test/*.o > /dev/null 2>&1
	@-rm ./lib/${LIBNAME} > /dev/null 2>&1
