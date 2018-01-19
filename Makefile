#Makefile for Building the processd and processclient applications
.PHONY: all server client clean

PROCESSD	= processd
PCLIENT		= processcli

GCC			= gcc
G++			= g++

SERVER_FILES	=	procreader.cpp main.cpp	
CLIENT_DIR      =   client/
INCLUDES		=	-I simpleini-master/
CPPFLAGS		=	-g $(INCLUDES) -pthread -std=c++11

all: server client

server:
	@echo "Building $@"
	$(G++) $(SERVER_FILES) $(CPPFLAGS) -o processd

client:
	@echo "Building $@"
	make -C $(CLIENT_DIR)

clean:
	@echo "Cleaning the binaries"
	@rm -rf processd
	make -C $(CLIENT_DIR) clean
