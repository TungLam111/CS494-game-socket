#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <vector>
#include <windows.h>
#include <cstdlib>
#pragma comment(lib, "ws2_32.lib")

#define minus "-"
#define add "+"
#define multiply "*"
#define divide "/"
#define modulo "%"

using namespace std;

struct Answer {
	string answer;
	bool isCorrect = false;
	int point = 0;

	void setYourAnswer(string ans) {
		answer = ans;
	}

	void setIsCorrect(bool check) {
		isCorrect = check;
	}
	
	void setPointForAnswer(int p) {
		point = p; // can be positive or negative
	}
};

struct Player {
	int client_socket;
	string client_name;
	int initial_point;
	vector<Answer> list_answer;
	bool isLose;

	Player(int id, string name) {
		client_socket = id;
		client_name = name;
		initial_point = 1;
		isLose = false;
	}

	int getTotalPoint() {
		int p = 0;
		for (int i = 0; i < list_answer.size(); i++) {
			p += list_answer[i].point;
		}
		return p + initial_point;
	}

	bool check3TimesWrong() {
		int count_answer = list_answer.size();
		if (count_answer >= 3) {
			if (list_answer[count_answer - 1].isCorrect == false && list_answer[count_answer - 2].isCorrect == false && list_answer[count_answer - 3].isCorrect == false) {
				return true;
			}
		}
		return false;
	}

	void reset() {
		client_socket = 0;
		client_name = "";
		initial_point = 1;
		isLose = false;
		list_answer.clear();
	}
};

int getRandomNumber(int lower, int upper)
{
    return (rand() % (upper - lower + 1)) + lower;
}

struct Race {
	int first_num;
	int second_num;
	string operation;
	int result;

	Race() {
		first_num = getRandomNumber(-10000, 10000);
		second_num = getRandomNumber(-10000, 10000);

		int random_sign = getRandomNumber(1, 5);
		switch (random_sign) {
		case 1:
			operation = minus;
			result = first_num - second_num;
			break;
		case 2:
			operation = add;
			result = first_num + second_num;
			break;
		case 3:
			operation = multiply;			
			result = first_num * second_num;
			break;

		case 4:
			operation = divide;			
			result = first_num / second_num;
			break;

		default:
			operation = modulo;			
			result = first_num % second_num;
			break;

		}
	}

	string getStatement() {
		return to_string(first_num) + " " + operation + " " + to_string(second_num);
	}
};

struct UtilService {

	Player createNewPlayer(int socket_id, string name) {
		return Player(socket_id, name);
	}

	bool checkOkName(vector< Player> arr, string check_name) {
		if (check_name.size() > 10 || check_name.empty()) {
			return false;
		}
		for (int i = 0; i < arr.size(); i++) {
			if (arr[i].client_name == check_name) {
				return false;
			}
		}
		return true;
	}

	void error(const char* msg)
	{
		perror(msg);
		exit(EXIT_FAILURE);
	}

	void success(const char* msg) {
		cout << msg << endl;
	}

	void printString(char arr[]) {
		for (int i = 0; i < strlen(arr); i++) {
			cout << arr[i];
		}
	}

	bool checkFull(vector<Player> arr, int n) {
		int count = 0;
		for (int i = 0; i < n; i++) {
			if (arr[i].client_socket != 0) {
				count++;
			}
		}
		if (count == n) {
			return TRUE;
		}
		return FALSE;
	}
};

void gameInfo() {
	cout << "Welcome to RACING ARENA" << endl;
	cout << "1. New game" << endl;
	cout << "2. Exit" << endl;
	cout << "Your pick: ";
}

struct RaceGame {
	UtilService util;
	int opt;
	vector<Player> client_sockets;
	vector<Race> race_list;
	int delay_race_time;
	int master_socket, addrlen, new_socket,
		max_clients, activity, valread, sd;
	int max_sd;
	struct sockaddr_in address;
	char buffer[1000]{};
	fd_set readfds;
	int port_server;
	int race_length;
	string statement;

	void setOpt() {
	    opt = TRUE;
	}

	void set_race_length(int length) {
		race_length = length;
	}

	void setUtil(UtilService sv) {
		util = sv;
	}

	void set_number_clients(int client) {
		max_clients = client;
	}

	void setPortNUmber(int port) {
		port_server = port;
	}

	void setDelayRaceTime(int sec) {
		delay_race_time = sec;
	}

	void displayAllPoints() {
		for (int i = 0; i < client_sockets.size(); i++) {
			cout << client_sockets[i].client_name << " " << client_sockets[i].getTotalPoint() << endl;
		}
	}

	int playGame() {

		// Initialise all client_socket[] to 0 so not checked
		for (int i = 0; i < max_clients; i++)
		{
			client_sockets.push_back(Player(0, ""));
		}

		// Create a master socket
		if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			util.error("ERROR: socket failed");
		}
		util.success("SUCCESS: create socket successful");

		if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
			sizeof(opt)) == SOCKET_ERROR) {
			util.error("ERROR: setsockopt");
		}
		util.success("SUCCESS: create socket successful");

		// Address configuration
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port_server);

		// Bind the socket to localhost port is port_server
		if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0)
		{
			util.error("ERROR: bind failed");
		}

		string bind_msg = "SUCCESS: Listener on port " + to_string(port_server) + "\n";
		util.success(bind_msg.c_str());

		// Try to specify maximum of 3 pending connections for the master socket
		if (listen(master_socket, 3) < 0)
		{
			util.error("ERROR: listen");
		}
		util.success("SUCCESS: listen \n");

		// Accept the incoming connection
		addrlen = sizeof(address);
		puts("Waiting for connections ...");

		bool isStartGame = false;

		while (TRUE)
		{
			// Clear the socket set
			FD_ZERO(&readfds);

			// Add master socket to set
			FD_SET(master_socket, &readfds);
			max_sd = master_socket;

			// Add child sockets to set
			for (int i = 0; i < max_clients; i++)
			{
				// Socket descriptor
				sd = client_sockets[i].client_socket;

				// If valid socket descriptor then add to read list
				if (sd > 0)
					FD_SET(sd, &readfds);

				// Highest file descriptor number, need it for the select function
				if (sd > max_sd)
					max_sd = sd;
			}

			// Wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
			activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

			if ((activity < 0) && (errno != EINTR))
			{
				cout << "select error" << endl;
			}

			// If something happened on the master socket , then its an incoming connection
			if (FD_ISSET(master_socket, &readfds))
			{
				if ((new_socket = accept(master_socket,
					(struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
				{
					util.error("ERROR: accept connection \n");
				}
				string ask_name_input = "Please input your name: ";
				char input_name[256]{};
				int size_input_name;
				do {
					int check_name_input = send(new_socket, ask_name_input.c_str(), ask_name_input.size(), 0);
					if (check_name_input <= 0) {
					}
					else {
						// Ask ok
						memset(input_name, 0, 256);
						size_input_name = recv(new_socket, input_name, 255, 0);
						if (size_input_name <= 0)
							util.error("ERROR: reading's result from socket");
						else {
							input_name[size_input_name - 1] = '\0';
						}
					}

				} while (util.checkOkName(client_sockets, std::string(input_name)) == false);

				Player newPlayer = util.createNewPlayer(new_socket, input_name);
				string register_msg = "REGISTER";
				send(newPlayer.client_socket, register_msg.c_str(), register_msg.size(), 0);

				// Inform user of socket number - used in send and receive commands
				char ipbuf[INET_ADDRSTRLEN];
				printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket, inet_ntop(AF_INET, &address.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(address.sin_port));

				// Greeting message
				string greet_msg = "Welcome to game " + to_string(new_socket);
				puts("Welcome message sent successfully");

				// Add new socket to array of sockets, like a queue
				for (int i = 0; i < max_clients; i++)
				{
					if (client_sockets[i].client_socket != 0) {
						send(client_sockets[i].client_socket, greet_msg.c_str(), greet_msg.size(), 0);
					}
					// If position is empty
					if (client_sockets[i].client_socket == 0)
					{
						client_sockets[i] = newPlayer;
						util.success(("Adding to list of sockets as " + to_string(i)).c_str());
						send(client_sockets[i].client_socket, greet_msg.c_str(), greet_msg.size(), 0);
						break;
					}
				}
			}

			if (isStartGame == false) {
				if (util.checkFull(client_sockets, max_clients)) {
					isStartGame = true;
					string start_msg = "START";
					for (int i = 0; i < max_clients; i++) {
						send(client_sockets[i].client_socket, start_msg.c_str(), start_msg.size(), 0);
					}
				}
			}

			// Receive answers before asking next questions
			Race lastes_race;
			if (!race_list.empty() && isStartGame == true) {
				Sleep(delay_race_time * 1000);
				lastes_race = race_list[race_list.size() - 1];
			}

			for (int i = 0; i < max_clients; i++)
			{
				sd = client_sockets[i].client_socket;
				if (FD_ISSET(sd, &readfds)) {
					memset(buffer, 0, 1000);
					valread = recv(sd, buffer, 999, 0);
					if (valread <= 0) {
						cout << "DISCONNECT FROM CLIENT: ";
						util.printString(buffer);
						cout << "\n";

						// Somebody disconnected , get his details and print
						getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
						char ipbuf[INET_ADDRSTRLEN];
						printf("Host disconnected , ip %s , port %d \n",
							inet_ntop(AF_INET, &address.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(address.sin_port));

						// Close the socket and mark as 0 in list for reuse
						send(sd, buffer, strlen(buffer), 0);
						client_sockets[i].reset();
					}
					else {
						if (isStartGame && !race_list.empty()) {
							buffer[valread - 1] = '\0';
							util.printString(buffer);

							Answer ans = Answer();
							ans.answer = string(buffer);
	    					if (ans.answer == to_string(lastes_race.result)) {
									ans.setIsCorrect(true);
									ans.setPointForAnswer(1);
							}
							else {
								ans.setIsCorrect(false);
								ans.setPointForAnswer(-1);
							}
							client_sockets[i].list_answer.push_back(ans);
						}
					}				
				}
			}

			if (isStartGame)
				displayAllPoints();
		    

			if (isStartGame) {
				// Countdown
				Sleep(delay_race_time * 1000);

				// Else its some IO operation on some other socket
				Race newRace = Race();
				cout << newRace.getStatement() << endl;
				cout << newRace.first_num << " " << newRace.second_num << endl;
				cout << newRace.result << endl;
				race_list.push_back(newRace);
				statement = race_list[race_list.size() - 1].getStatement();
			}

			for (int i = 0; i < max_clients; i++)
			{
				sd = client_sockets[i].client_socket;
				if (isStartGame && !race_list.empty()) {
				    int tn = send(sd, statement.c_str(), statement.size(), 0);
				    if (tn <= 0)
						util.error("ERROR: writing to socket");
				}	
			}			
		
		
			
		}

		// Remove the listening socket from the master file descriptor set and close it to prevent anyone else trying to connect.
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
};

int setUpServer(int port_server)
{
	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	UtilService util = UtilService();
	if (wsOk != 0)
	{
		util.error("ERROR: Can't Initialize winsock! Quitting");
	}
	else {
		util.success("SUCCESS: Initialize winsock!");
	}

	int option = -1;
	RaceGame game;

	do {
		gameInfo();
		cin >> option;

		if (option == 2) {
			return 1;
		} 
		
		if (option == 1) {
			game = RaceGame();
			game.setUtil(util);
			game.setOpt();
			game.setPortNUmber(port_server);
			game.setDelayRaceTime(5);
			game.set_number_clients(2);
			game.set_race_length(10);
			game.playGame();
		}
	} while (option != 1 && option != 2);

	return 1;
}

struct RaceClient {
	UtilService util;

	int port_server, n;
	SOCKET sockfd;

	struct sockaddr_in address;
	char* host_name;
	char buffer[256];
	bool isRegister = false;
	bool isGame = false;

	void initialize() {
		isRegister = false;
		isGame = false;
	}

	void setUtilService(UtilService sv) {
		util = sv;
	}

	void setPortNumber(int port) {
		port_server = port;
	}

	void setHostName(char* host) {
		host_name = host;
	}

	int joinAndPlay() {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == INVALID_SOCKET) {
			util.error("ERROR: opening socket");
		}

		address.sin_family = AF_INET;
		address.sin_port = htons(port_server);
		inet_pton(AF_INET, host_name, &address.sin_addr);

		// Connect to server
		int connResult = connect(sockfd, (sockaddr*)&address, sizeof(address));
		if (connResult == SOCKET_ERROR)
		{
			cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
			closesocket(sockfd);
			WSACleanup();
			exit(EXIT_FAILURE);
		}

		// Loop until registered
		while (isRegister == false) {
			// Reset buffer
			memset(buffer, 0, 256);
			n = recv(sockfd, buffer, 255, 0);
			if (n <= 0)
				util.error("ERROR: reading's result from socket");

			// If receive ok, read to check 
			if (string(buffer) == "REGISTER") {
				isRegister = true;
				break;
			}

			util.printString(buffer);
			cout << endl;

			memset(buffer, 0, 256);
			fgets(buffer, 255, stdin);
			n = send(sockfd, buffer, strlen(buffer), 0);
			if (n <= 0)
				util.error("ERROR: writing to socket");
		}

		// Loop until game start
		while (isGame == false) {
			memset(buffer, 0, 256);
			n = recv(sockfd, buffer, 255, 0);

			if (n <= 0)
				util.error("ERROR: reading's result from socket");
			if (string(buffer) == "START") {
				isGame = true;
			}
			util.printString(buffer);
			cout << endl;
		}

		// In game
		while (isGame) {
			// Question
			memset(buffer, 0, 256);
			n = recv(sockfd, buffer, 255, 0);
			cout << "Question: ";
			util.printString(buffer);

			if (string(buffer) == "END") {
				isGame = false;
			}
			else {
				cout << "\nPlease enter the message: ";
				memset(buffer, 0, 256);
				fgets(buffer, 255, stdin);
				n = send(sockfd, buffer, strlen(buffer), 0);
			}			
		}

		closesocket(sockfd);
		WSACleanup();
		return 0;
	}
};

int setUpClient(char* host_name ,int port_server)
{
	string ipAddress = "127.0.0.1";		
	cout << host_name << endl;

	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);

	UtilService util = UtilService();
	if (wsOk != 0)
	{
		util.error("ERROR: Can't Initialize winsock! Quitting");
	}
	else {
		util.success("SUCCESS: Initialize winsock!");
	}

	RaceClient client = RaceClient();
	client.setUtilService(util);
	client.initialize();
	client.setHostName(host_name);
	client.setPortNumber(port_server);
	return client.joinAndPlay();
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