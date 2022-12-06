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
#include <codecvt> // for std::codecvt_utf8
#include <locale>  // for std::wstring_convert
#include <iomanip>

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
	int playStatus; // 0 - Playing, 1 - Win, -1 - Lose

	Player(int id, string name) {
		client_socket = id;
		client_name = name;
		initial_point = 1;
		playStatus = 0;
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

	void setLose() {
		client_socket = 0;
		playStatus = -1;
	}

	string status() {
		if (playStatus == 0) {
			return "Playing";
		}
		if (playStatus == 1) {
			return "Win";
		}
		return "Lose";
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
	std::time_t time;

	Race() {
		first_num = getRandomNumber(-10, 10);
		second_num = getRandomNumber(-10, 10);

		int random_sign = getRandomNumber(1, 5);
		time = std::time(0);

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
	int current_round = 0;
	bool isStartGame = false;

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
		cout << "/---------Playboard---------/" << endl;
		for (int i = 0; i < client_sockets.size(); i++) {
			cout << client_sockets[i].client_name << " " << client_sockets[i].getTotalPoint() << " " << client_sockets[i].status() << endl;
		}
		cout << "/---------------------------/" << endl;
	}

	int remainPlaying() {
		int count = 0;
		for (int i = 0; i < client_sockets.size(); i++) {
			if (client_sockets[i].client_socket != 0 && client_sockets[i].playStatus == 0) {
				++count;
			}
		}
		return count;
	}

	bool checkGameContinue() {
		// Wait until all players ans or drop
		for (int i = 0; i < client_sockets.size(); i++) {
			if (client_sockets[i].playStatus == 0 && client_sockets[i].list_answer.size() != current_round) {
				return false;
			}
		}
		return true;
	}

	bool checkEndGame() {
		if (isStartGame && remainPlaying() == 1) {
			return true; // Winner
		}

		// End of race
		if (isStartGame && current_round == race_length) {
			return true;
		}

		return false;
	}

	~RaceGame()
	{
		cout << "RaceGame Destructor executed" << endl;
	}

	int playGame() {

		// Initialze winsock
		WSADATA wsData;
		WORD ver = MAKEWORD(2, 2);

		int wsOk = WSAStartup(ver, &wsData);
		if (wsOk != 0)
		{
			util.error("ERROR: Can't Initialize winsock! Quitting");
		}
		else {
			util.success("SUCCESS: Initialize winsock!");
		}

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

			// If something happened on the master socket , then its an incoming connection + register
			if (FD_ISSET(master_socket, &readfds))
			{
				if ((new_socket = accept(master_socket,
					(struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
				{
					util.error("ERROR: accept connection \n");
				}

				string ask_name_input = "Type your name: ";
				char input_name[256]{};
				int size_input_name;

				// recv ORDER INTER ROOM
				memset(buffer, 0, 256);
				recv(new_socket, buffer, 255, 0);
				util.printString(buffer);
				cout << endl;

				do {

					// send ASK NICKNAME
					int check_name_input = send(new_socket, ask_name_input.c_str(), ask_name_input.size(), 0);

					// recv NICKNAME
					memset(input_name, 0, 256);
					size_input_name = recv(new_socket, input_name, 255, 0);
					input_name[size_input_name - 1] = '\0';
				} while (util.checkOkName(client_sockets, std::string(input_name)) == false);

				Player newPlayer = util.createNewPlayer(new_socket, input_name);

				// send REGISTER
				string register_msg = "REGISTER";
				send(newPlayer.client_socket, register_msg.c_str(), register_msg.size(), 0);

				char ipbuf[INET_ADDRSTRLEN];
				printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket, inet_ntop(AF_INET, &address.sin_addr, ipbuf, sizeof(ipbuf)), ntohs(address.sin_port));

				// Greeting message
				string greet_msg = "Welcome to game " + to_string(new_socket);
				puts("Welcome message sent successfully");

				// set new players to array
				for (int i = 0; i < max_clients; i++)
				{

					if (client_sockets[i].client_socket == 0) {
						client_sockets[i] = newPlayer;
						util.success(("Adding to list of sockets as " + to_string(i)).c_str());
						break;
					}
				}

				for (int i = 0; i < max_clients; i++)
				{
					if (client_sockets[i].client_socket != 0) {
						// send ENTER ROOM - notify new player
						send(client_sockets[i].client_socket, greet_msg.c_str(), greet_msg.size(), 0);
					}
				}
				cout << "Player is registered" << endl;

			}

			// Check if game's ok to start
			if (isStartGame == false && util.checkFull(client_sockets, max_clients)) {
				for (int i = 0; i < max_clients; i++) {
					// recv Ready?
					if (client_sockets[i].client_socket != 0) {
						string msg;
						isStartGame = true;
						string start_msg = "START";
						string delay_time_str = to_string(delay_race_time);
						msg = start_msg + " " + delay_time_str;
						send(client_sockets[i].client_socket, msg.c_str(), msg.size(), 0);
					}
				}
			}

			// Receive answers before asking next questions
			Race last_race;
			if (!race_list.empty() && isStartGame == true) {
				last_race = race_list[race_list.size() - 1];
				cout << "Correct_answer " << last_race.result << endl;
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
						client_sockets[i].setLose();
					}
					else {
						if (isStartGame && !race_list.empty()) {
							buffer[valread - 1] = '\0';

							// Time complete
							std::time_t t = std::time(0);

							double diff_time = difftime(t, last_race.time);
							util.printString(buffer);
							cout << endl;

							Answer ans = Answer();
							ans.answer = string(buffer);

							if (diff_time < delay_race_time * 4) {
								if (ans.answer == to_string(last_race.result)) {
									ans.setIsCorrect(true);
									ans.setPointForAnswer(1);
								}
								else {
									ans.setIsCorrect(false);
									ans.setPointForAnswer(-1);
								}
							}
							else {
								ans.setIsCorrect(false);
								ans.setPointForAnswer(-1);
							}
							client_sockets[i].list_answer.push_back(ans);

							// Check 3 consecutive incorrect ans
							if (client_sockets[i].check3TimesWrong()) {
								client_sockets[i].setLose();
							}

						}
					}
				}
			}


			if (checkGameContinue()) {
				// send correct ans to clients
				if (current_round != 0) {
					// refinement 



					string res = "RESULT" + to_string(last_race.result);
					for (int i = 0; i < client_sockets.size(); i++) {
						if (client_sockets[i].client_socket != 0) {
							send(client_sockets[i].client_socket, res.c_str(), res.size(), 0);
						}
					}
				}

				if (checkEndGame()) {
					isStartGame = false;
					break;
				}
			}

			if (isStartGame && checkGameContinue()) {
				// Countdown
				cout << "New racing comming" << endl;
				Sleep(delay_race_time * 2 * 1000);
				Race newRace = Race();
				race_list.push_back(newRace);
				current_round++;
				statement = race_list[race_list.size() - 1].getStatement();

				cout << "/------------------/" << endl;
				cout << "/ Race " << current_round << endl;
				cout << "/------------------/" << endl;

				cout << statement << endl;
				for (int i = 0; i < max_clients; i++)
				{
					sd = client_sockets[i].client_socket;
					if (sd != 0) {
						int tn = send(sd, statement.c_str(), statement.size(), 0);
						if (tn <= 0) {
							util.error("ERROR: writing to socket");
						}
					}
				}
			}
		}

		string msg_end = "END";
		for (int i = 0; i < client_sockets.size(); i++) {
			if (client_sockets[i].client_socket != 0) {
				send(client_sockets[i].client_socket, msg_end.c_str(), msg_end.size(), 0);
			}
		}

		// Remove the listening socket from the master file descriptor set and close it to prevent anyone else trying to connect.
		cout << "master_socket = " << master_socket << endl;
		FD_CLR(master_socket, &readfds);
		closesocket(master_socket);

		// Message to let users know what's happening.
		string msg = "Server is shutting down. Goodbye\r\n";

		cout << "fd count = " << readfds.fd_count << endl;
		while (readfds.fd_count > 0)
		{
			// Get the socket number
			int sock = readfds.fd_array[0];

			// Send the goodbye message
			send(sock, msg.c_str(), msg.size() + 1, 0);

			// Remove it from the master file list and close the socket
			FD_CLR(sock, &readfds);
			closesocket(sock);

			cout << "Close client socket " << sock << endl;
		}

		// Cleanup winsock
		WSACleanup();
		system("pause");
		return 0;
	}
};

int setUpServer()
{
	int option = -1;
	RaceGame* game;
	UtilService util = UtilService();

	while (true) {
		gameInfo();
		cin >> option;

		if (option == 2) {
			break;
		}

		if (option == 1) {
			game = new RaceGame();
			game->setUtil(util);
			game->setOpt();
			game->setPortNUmber(8000);
			game->setDelayRaceTime(3);
			game->set_number_clients(3);
			game->set_race_length(2);
			int game_status = game->playGame();
			delete game;
		}
	}
	return 1;
}

struct RaceClient {
	UtilService util;

	int port_server, n;
	SOCKET sockfd;

	struct sockaddr_in address;
	char* host_name;
	char buffer[256];
	char message[1020];

	bool isRegister = false;
	bool isGame = false;

	string host_name2;

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

	void setHostName2(string host) {
		host_name2 = host;
	}

	~RaceClient()
	{
		cout << "RaceClient Destructor executed" << endl;
	}

	vector<string> splitString(string strr) {
		std::stringstream test(strr);
		std::string segment;
		std::vector<std::string> seglist;

		while (std::getline(test, segment, '_'))
		{
			seglist.push_back(segment);
		}

		return seglist;
	}

	bool checkHasWinnerYet(vector<int> status, int color) {
		for (int i = 0; i < status.size(); i++) {
			if (status[i] == 100) {
				return true && (i != color);
			}
		}
		return false;
	}

	string rankPrint(int k) {
		if (k <= -100) {
			return "Loseeeeeeeeeeeeeeeeeeeeeeeeeee";
		}
		else if (k >= 100) {
			return "Winnnnnnnnnnnnnnnnnnnnnnnnnnnn";
		}

		string t = "";
		for (int i = 0; i < k; i++) {
			t = t + "========";
		}

		return t;
	}

	int joinAndPlay2() {
		// Initialze winsock
		WSADATA wsData;
		WORD ver = MAKEWORD(2, 2);

		int wsOk = WSAStartup(ver, &wsData);
		if (wsOk != 0)
		{
			util.error("ERROR: Can't Initialize winsock! Quitting");
		}
		else {
			util.success("SUCCESS: Initialize winsock!");
		}

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == INVALID_SOCKET) {
			util.error("ERROR: opening socket");
		}

		address.sin_family = AF_INET;
		address.sin_port = htons(port_server);
		inet_pton(AF_INET, host_name2.c_str(), &address.sin_addr);

		// Connect to server
		cout << "Waiting for connection" << endl;
		int connResult = connect(sockfd, (sockaddr*)&address, sizeof(address));
		if (connResult == SOCKET_ERROR)
		{
			cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
			closesocket(sockfd);
			WSACleanup();
			exit(EXIT_FAILURE);
		}

		memset(message, 0, 1020);
		recv(sockfd, message, 1019, 0);

		util.printString(message);
		cout << endl;

		int color = NULL;
		int raceLength = NULL;
		int lastStatus = 1;
		int isPlaying = true;
		int port = port_server + 1;
		int currentRace = 0;
		while (true) {
			if (isPlaying == false) {
				break;
			}

			bool isLoggedInSuccessfully = false;
			while (isLoggedInSuccessfully == false) {
				cout << "Input name: " << endl;
				memset(message, 0, 1020);
				fgets(message, 1019, stdin);

				send(sockfd, message, strlen(message), 0);

				memset(message, 0, 1020);
				recv(sockfd, message, 1019, 0);
				string msg = string(message);

				if (msg.substr(0, 2) == "ok") {
					vector<string> data = splitString(msg);
					color = atoi(data[1].c_str());
					cout << "Registration completed successfully with order: " << color << endl;
					isLoggedInSuccessfully = true;
				}
				else {
					cout << msg << endl;
				}
			}

			cout << "------------------------------------ Wait for another player" << endl;

			memset(message, 0, 1020);
			recv(sockfd, message, 1019, 0);
			raceLength = atoi(message);

			Sleep(2 * 1000);

			cout << "Race length: " << to_string(raceLength) << endl;
			string msg_ready = "Ready";
			send(sockfd, msg_ready.c_str(), msg_ready.size(), 0);

			while (true) {
				memset(message, 0, 1020);
				recv(sockfd, message, 1019, 0);

				cout << "[?] Your question: ";
				util.printString(message);
				cout << endl;
				currentRace++;

				cout << "[>] Your answer: ";
				memset(message, 0, 1020);
				fgets(message, 1019, stdin);
				send(sockfd, message, strlen(message), 0);

				cout << "------------------------------------ Waiting for another racer" << endl;
				memset(message, 0, 1020);
				recv(sockfd, message, 1019, 0);

				util.printString(message);
				cout << endl;

				vector<string> gameStatus = splitString(string(message));

				vector<int> status;
				for (int i = 0; i < gameStatus.size(); i++) {
					status.push_back(atoi(gameStatus[i].c_str()));
				}

				int playerStatus = status[color];

				cout << "[>] Your score: " << playerStatus << endl;

				if (playerStatus == -100 || checkHasWinnerYet(status, color)) {
					cout << "You lose" << endl;

				}
				else if (playerStatus == 100) {
					cout << "You win" << endl;
				}

				else {
					if (playerStatus == lastStatus + 1) {
						cout << "[v] CORRECT ANSWER" << endl;
					}
					else if (playerStatus <= lastStatus) {
						cout << "[x] WRONG ANSWER" << endl;
					}
					else if (playerStatus > lastStatus) {
						cout << "Bonus for Fastest Racer in this round" << endl;
					}
				}


				lastStatus = playerStatus;



				for (int i = 0; i < status.size(); i++) {
					if (i == color) {
						cout << right << setfill('.') << setw(30) << "you" << " " << rankPrint(status[i]) << endl;
					}
					else {
						cout << right << setfill('.') << setw(30) << "player " + to_string(i) << " " << rankPrint(status[i]) << endl;
					}
				}

				if (playerStatus == 100) {
					isPlaying = false;
					string msg_win = "Win game";
					send(sockfd, msg_win.c_str(), msg_win.size(), 0);
					break;
				}
				else if (playerStatus == -100 || checkHasWinnerYet(status, color)) {
					isPlaying = false;
					string msg_win = "Out game";
					send(sockfd, msg_win.c_str(), msg_win.size(), 0);
					break;
				}
				else {

					string msg_win = "Next question";
					send(sockfd, msg_win.c_str(), msg_win.size(), 0);

					cout << "/------ Next round ------/" << endl;
				}
			}
		}

		cout << "Goodbye!" << endl;

		closesocket(sockfd);
		WSACleanup();
		return 0;
	}

	int joinAndPlay() {

		// Initialze winsock
		WSADATA wsData;
		WORD ver = MAKEWORD(2, 2);

		int wsOk = WSAStartup(ver, &wsData);
		if (wsOk != 0)
		{
			util.error("ERROR: Can't Initialize winsock! Quitting");
		}
		else {
			util.success("SUCCESS: Initialize winsock!");
		}


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

		// send ORDER INTER ROOM
		string msg_enter_room = "HI, I'am new";
		send(sockfd, msg_enter_room.c_str(), msg_enter_room.size(), 0);

		// Loop until registered
		while (isRegister == false) {

			// recv ASK NICKNAME
			memset(buffer, 0, 256);
			recv(sockfd, buffer, 255, 0);

			if (string(buffer) != "REGISTER") {
				util.printString(buffer);
				cout << endl;
				memset(buffer, 0, 256);
				fgets(buffer, 255, stdin);

				// send NICKNAME 
				send(sockfd, buffer, strlen(buffer), 0);
			}
			else {
				isRegister = true;
				cout << "Successful register nick name" << endl;
				break;
			}
		}


		// send ENTER ROOM 
		if (isRegister && isGame == false) {
			// recv ENTER ROOM - notify new player
			memset(buffer, 0, 256);
			n = recv(sockfd, buffer, 255, 0);
			util.printString(buffer); // Welcome to game ...
			cout << endl;
		}

		// Loop until game start
		cout << "Player is registered" << endl;
		while (isGame == false) {
			// recv ready?
			memset(buffer, 0, 256);
			n = recv(sockfd, buffer, 255, 0);
			string data = string(buffer);

			if (data.substr(0, 5) != "START") {
				string mssg = "";
				// send NICKNAME 
				send(sockfd, mssg.c_str(), mssg.size(), 0);
			}
			else {
				cout << "START" << endl;
				isGame = true;
				break;
			}
		}

		string data;
		string time_at;
		// In game
		while (isGame) {
			// Question
			memset(buffer, 0, 256);
			n = recv(sockfd, buffer, 255, 0);
			string data = string(buffer);

			if (data == "END") {
				isGame = false;
			}
			else if (data.substr(0, 6) == "RESULT") {
				cout << "Correct answer: " << data.substr(6, data.size()) << endl;
			}
			else {
				cout << "Question: " << data << endl;
				// Type answer
				cout << "Please enter the answer: ";
				memset(buffer, 0, 256);
				fgets(buffer, 255, stdin);
				data = string(buffer);
				n = send(sockfd, data.c_str(), data.size(), 0);
			}
		}

		cout << "Goodbye!" << endl;

		closesocket(sockfd);
		WSACleanup();
		return 0;
	}
};

int setUpClient(int port)
{
	string ipAddress = "127.0.0.1";

	UtilService util = UtilService();

	RaceClient client = RaceClient();
	client.setUtilService(util);
	client.initialize();
	client.setHostName2(ipAddress);
	client.setPortNumber(port);
	return client.joinAndPlay2();
}

int client(char* host, int port) {
	string ipAddress = "127.0.0.1";
	cout << host << endl;

	UtilService util = UtilService();

	RaceClient client = RaceClient();
	client.setUtilService(util);
	client.initialize();
	client.setHostName(host);
	client.setPortNumber(port);
	return client.joinAndPlay();
}

int main(int argc, char* argv[]) {

	// client: console.exe client hostname port_server
	if (argc == 3) {
		string side_name = string(argv[1]);
		if (side_name == "client") {
			return setUpClient(atoi(argv[2]));
		}
		return 0;
	}
	else {
		cout << "Wrong command line" << endl;
		return 0;
	}

	return 1;
}