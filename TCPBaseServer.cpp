//============================================================================
// Name        : TCPBaseServer.cpp
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
#include "ServerProgram.h"

unordered_map<string, pair<string, bool>> users;
unordered_map<string, string> msgs;

void baseTCPProtocolS(int csoc)
{
	cout << "baseTCPProtocol Started\n";

	// read 10 bytes from client
	// char buf[10];
	// recvLoop(csoc, buf, 10);
	serverClientInteraction(csoc, users, msgs);

	// send 10 bytes to client
	// send(csoc, buf, 10, 0);

	cout << "baseTCPProtocol Ended\n";
}

int main(int argc, char *argv[])
{

	sockaddr_in sin;  // local socket address
	sockaddr_in fsin; // client socket address

	int ssoc;	 // server socket -passive
	int commsoc; // communication socket -active

	int sckt = 50005;

	cout << "TCPBaseServer" << endl; // prints TCPBaseServer

	// init the local address
	memset(&sin, 0, sizeof(sin));

	// set the family, address, port
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY; // use all addresses
	sin.sin_port = htons((unsigned short)sckt);

	// create the server socket
	ssoc = socket(PF_INET, SOCK_STREAM, 0);
	if (ssoc < 0)
	{
		cerr << "Socket error\n";
		return -1;
	}

	// bind to local port
	if (bind(ssoc, (sockaddr *)&sin, sizeof(sin)) < 0)
	{
		cerr << "Cannot bind\n";
		return -1;
	}

	// make passive and set backlog to 5
	if (listen(ssoc, 5) < 0)
	{
		cerr << "Cannot make passive\n";
		return -1;
	}

	while (true)
	{
		cout << "Listening on " << sckt << "..." << endl;

		// wait for connection
		unsigned int alen = sizeof(fsin);
		commsoc = accept(ssoc, (sockaddr *)&fsin, &alen);
		if (commsoc < 0)
		{
			cerr << "Error on accept\n";
			break;
		}

		// run the application protocol
		baseTCPProtocolS(commsoc);

		// close the communication socket
		close(commsoc);
	}

	close(ssoc);

	return 0;
}
