#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>

#pragma comment (lib, "ws2_32.lib")

#define TRUE   1 
#define FALSE  0 
#define PORT 8888 

using namespace std;

void error(const char* msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main()
{
    std::cout << "Hello World!\n";

    int max_client;

    SOCKET master_socket;
    char buffer[1025];

    fd_set readfds;

    //create a master socket 
    master_socket = socket(AF_INET, SOCK_STREAM, 0);

}