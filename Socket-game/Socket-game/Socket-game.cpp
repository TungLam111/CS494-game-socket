#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void success(const char* msg) {
	cout << msg << endl;
}

#define TRUE 1
#define FALSE 0

int setUpServer(int port_server)
{
	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		error("ERROR: Can't Initialize winsock! Quitting");
	}
	else {
		success("SUCCESS: Initialize winsock!");
	}

	int opt = TRUE;
	int master_socket, addrlen, new_socket, client_socket[30],
		max_clients = 30, activity, i, valread, sd;
	int max_sd;
	struct sockaddr_in address;

	char buffer[1025] {}; //data buffer of 1K

	//set of socket descriptors
	fd_set readfds;

	//a message
	const char* message = "ECHO Daemon v1.0 \r\n";

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; i++)
	{
		client_socket[i] = 0;
	}

	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		error("ERROR: socket failed");
	}
	else {
		success("SUCCESS: create socket successful");
	}

	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
		sizeof(opt)) == SOCKET_ERROR) {
		error("ERROR: setsockopt");
	}
	else {
		success("SUCCESS: create socket successful");
	}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port_server);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0)
	{
		error("ERROR: bind failed");
	} 

	printf("Listener on port %d \n", port_server);

	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
	{
		error("ERROR: listen");
	}
    printf("SUCCESS: listen \n");
	

	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	while (TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		//add child sockets to set
		for (i = 0; i < max_clients; i++)
		{
			//socket descriptor
			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			//highest file descriptor number, need it for the select function
			if (sd > max_sd)
				max_sd = sd;
		}

		//wait for an activity on one of the sockets , timeout is NULL ,
		//so wait indefinitely
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error \n");
		}

		//If something happened on the master socket ,
		//then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket,
				(struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
			{
				error("accept");
			}

			//inform user of socket number - used in send and receive commands
			char ipbuf[INET_ADDRSTRLEN];
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntop(AF_INET, &address.sin_addr, ipbuf, sizeof(ipbuf)) , ntohs
				(address.sin_port));

			//send new connection greeting message
			if (send(new_socket, message, strlen(message), 0) != strlen(message))
			{
				perror("send");
			}

			puts("Welcome message sent successfully");

			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				//if position is empty
				if (client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d\n", i);

					break;
				}
			}
		}

		//else its some IO operation on some other socket
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds))
			{
				//Check if it was for closing , and also read the
				//incoming message
				if ((valread = recv(sd, buffer, strlen(buffer), 0)) == 0)
				{
					//Somebody disconnected , get his details and print
					getpeername(sd, (struct sockaddr*)&address, \
						(socklen_t*)&addrlen);
					char ipbuf[INET_ADDRSTRLEN];
					printf("Host disconnected , ip %s , port %d \n",
						inet_ntop(AF_INET, &address.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(address.sin_port));

					//Close the socket and mark as 0 in list for reuse
				    send(sd, buffer, strlen(buffer), 0);
					client_socket[i] = 0;
				}

				//Echo back the message that came in
				else
				{
					//set the string terminating NULL byte on the end
					//of the data read
					buffer[valread] = '\0';
					send(sd, buffer, strlen(buffer), 0);
				}
			}
		}
	}



	// Remove the listening socket from the master file descriptor set and close it
    // to prevent anyone else trying to connect.
	FD_CLR(master_socket, &readfds);
	closesocket(master_socket);

	// Message to let users know what's happening.
	string msg = "Server is shutting down. Goodbye\r\n";

	while (readfds.fd_count > 0)
	{
		// Get the socket number
		SOCKET sock = readfds.fd_array[0];

		// Send the goodbye message
		send(sock, msg.c_str(), msg.size() + 1, 0);

		// Remove it from the master file list and close the socket
		FD_CLR(sock, &readfds);
		closesocket(sock);
	}

	// Cleanup winsock
	WSACleanup();

	system("pause");

	return 0;
}

int setUpClient(char* host_name ,int port_server)
{
	string ipAddress = "127.0.0.1";		
	cout << host_name << endl;

	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		error("ERROR: Can't Initialize winsock! Quitting");
	}
	else {
		success("SUCCESS: Initialize winsock!");
	}

	SOCKET sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == INVALID_SOCKET) {
		error("ERROR opening socket");
	}

	int portno, n;

	portno = port_server;

	struct sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(portno);
	inet_pton(AF_INET, host_name, &hint.sin_addr);

	// Connect to server
	int connResult = connect(sockfd, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(sockfd);
		WSACleanup();
		exit(EXIT_FAILURE);
	}

	char buffer[256];
	cout << "Please enter the message: ";
	memset(buffer, 0, 256);
	fgets(buffer, 255, stdin);
	n = send(sockfd, buffer, strlen(buffer), 0);
	if (n < 0)
		error("ERROR writing to socket");
	memset(buffer, 0, 256);
	n = recv(sockfd, buffer, 255, 0);
	if (n < 0)
		error("ERROR reading from socket");
	printf("%s\n", buffer);

	closesocket(sockfd);
	WSACleanup();
	return 0;
}
int main(int argc, char* argv[]) {

	// client: console.exe client hostname port_server
	if (argc == 4) {
		string side_name = string(argv[1]);
		if (side_name != "client") {
			return 0;
		}
		char* host_name = argv[2];
		int port_server = atoi(argv[3]);
		return setUpClient(host_name, port_server);
	}
	// server: console.exe server port_server
	else if (argc == 3) {
		string side_name = string(argv[1]);
		if (side_name != "server") {
			return 0;
		}
		int port_server = atoi(argv[2]);
		return setUpServer(port_server);
	}
	else {
		cout << "Wrong command line" << endl;
		return 0;
	}

	return 1;
}