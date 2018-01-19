#Makfile for Building the client
.PHONY: all clean
CLIENT_FILE		=	client.cpp
G++				=	g++
INCLUDES		=
CXXFLAGS		=   -std=c++11
PCLIENT			=	processcli


all:	client

client:
	@echo "Building the client"
	$(G++)	$(CLIENT_FILE)	$(CXXFLAGS) -o $(PCLIENT)

clean:
	@echo "Cleaning the client"
	rm -rf client/processcli
