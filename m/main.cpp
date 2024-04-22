//============================================================================
// Name        : TCPBaseServer.cpp
// Author      : NMJ
// Version     :
// Copyright   : (c) NMJ
// Description : Hello World in C++, Ansi-style
//============================================================================

/*#include <iostream>
#include <cstring>
#include <cerrno>
using namespace std;

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include "middleware/MiddlewareProgram.h"

void baseTCPProtocolS(int csoc, int ssoc)
{
	cout << "baseTCPProtocol Started\n";

	// read 10 bytes from client
	// char buf[10];
	// recvLoop(csoc, buf, 10);
	middlewareClientInteraction(csoc, ssoc);

	// send 10 bytes to client
	// send(csoc, buf, 10, 0);

	cout << "baseTCPProtocol Ended\n";
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		cerr << "Usage: " << argv[0] << " serverAddress serverPort middlewarePort\n";
		return -1;
	}

	addrinfo hints, *res0; // client socket address
	int commsocS;		   // communication socket server -active
	int error;
	const char *cause = NULL;

	cout << "TCPMiddleware" << endl; // prints

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
	commsocS = -1;
	commsocS = socket(res0->ai_family, res0->ai_socktype,
					  res0->ai_protocol);

	if (commsocS < 0)
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
	if (connect(commsocS, res0->ai_addr, res0->ai_addrlen) < 0)
	{
		cerr << "Cannot connect\n";
		return -1;
	}

	cout << "Connected to " << argv[1] << " on " << argv[2] << endl;

	sockaddr_in sin;  // local socket address
	sockaddr_in fsin; // client socket address

	int ssoc;	  // server socket -passive
	int commsocC; // communication socket -active

	int sckt = atoi(argv[3]);

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
		commsocC = accept(ssoc, (sockaddr *)&fsin, &alen);
		if (commsocC < 0)
		{
			cerr << "Error on accept\n";
			break;
		}

		// run the application protocol
		baseTCPProtocolS(commsocC, commsocS);

		// close the communication socket
		close(commsocC);
	}

	close(ssoc);

	// close the socket
	close(commsocS);

	return 0;
}*/

#include "google/cloud/storage/client.h"
#include <iostream>

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cerr << "Missing bucket name.\n";
		std::cerr << "Usage: quickstart <bucket-name>\n";
		return 1;
	}
	std::string const bucket_name = argv[1];

	// Create aliases to make the code easier to read.
	namespace gcs = ::google::cloud::storage;

	// Create a client to communicate with Google Cloud Storage. This client
	// uses the default configuration for authentication and project id.
	auto client = gcs::Client();

	auto writer = client.WriteObject(bucket_name, "quickstart.txt");
	writer << "Hello World!";
	writer.Close();
	if (!writer.metadata())
	{
		std::cerr << "Error creating object: " << writer.metadata().status()
				  << "\n";
		return 1;
	}
	std::cout << "Successfully created object: " << *writer.metadata() << "\n";

	auto reader = client.ReadObject(bucket_name, "quickstart.txt");
	if (!reader)
	{
		std::cerr << "Error reading object: " << reader.status() << "\n";
		return 1;
	}

	std::string contents{std::istreambuf_iterator<char>{reader}, {}};
	std::cout << contents << "\n";

	return 0;
}
