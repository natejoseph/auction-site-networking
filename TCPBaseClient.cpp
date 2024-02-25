//============================================================================
// Name        : TCPBaseClient.cpp
// Author      : NMJ
// Version     :
// Copyright   : (c) NMJ
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <cstring>
#include <cerrno>
using namespace std;

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#include "client/ClientProgram.h"

void baseTCPProtocolC(int csoc) // app protocol!
{
	cout << "baseTCPProtocol Started\n";

	clientInterface(csoc);
	// send 10 bytes to server
	// char *d = "1234567890";
	// send(csoc, d, 10, 0);

	// read 10 bytes from server
	// char buf[10];
	// recvLoop(csoc, buf, 10);

	cout << "baseTCPProtocol Ended\n";
}

int main(int argc, char *argv[])
{

	addrinfo hints, *res0; // client socket address
	int commsoc;		   // communication socket -active
	int error;
	const char *cause = NULL;

	cout << "TCPClient" << endl; // prints TCPBaseServer

	// set the hints
	// use AF_INET - IPv4 addressing
	// use SOCK_STREAM - TCP protocol
	// return info in res0
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(argv[1], NULL, &hints, &res0);
	if (error)
	{
		cerr << "getaddrinfo error\n";
		return -1;
	}

	// create the socket
	commsoc = -1;
	commsoc = socket(res0->ai_family, res0->ai_socktype,
					 res0->ai_protocol);

	if (commsoc < 0)
	{
		cerr << "cannot get socket\n";
		return -1;
	}

	// set the port number
	// first cast to sockaddr_in (IPv4 style addr)
	// then set port, must use host-to-network order conversion
	struct sockaddr_in *inaddr = (struct sockaddr_in *)(res0->ai_addr);
	inaddr->sin_port = htons(atoi(argv[2]));

	// connect to the server
	if (connect(commsoc, res0->ai_addr, res0->ai_addrlen) < 0)
	{
		cerr << "Cannot connect\n";
		return -1;
	}

	// run the application protocol
	baseTCPProtocolC(commsoc);

	// close the socket
	close(commsoc);

	return 0;
}
