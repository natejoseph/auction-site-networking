#include "ServerProgram.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

int recvLoop(int csoc, char *data, const int size)
{
    int nleft, nread;
    char *ptr;

    ptr = data;
    nleft = size;
    while (nleft > 0)
    {
        nread = recv(csoc, ptr, nleft, 0);
        if (nread < 0)
        {
            cerr << "Read Error\n";
            break;
        }
        else if (nread == 0)
        {
            cerr << "Socket closed\n";
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return size - nleft;
}

char *readClientCommand(int csoc)
{
    char buf[4];
    recvLoop(csoc, buf, 4);
    int size = atoi(buf);
    size -= 4;
    char *data = new char[size];
    recvLoop(csoc, data, size);
    cout << "Client Command: " << data << endl;

    return data;
}

void serverClientInteraction(int csoc, unordered_map<string, pair<string, bool>> &users, unordered_map<string, string> &msgs)
{
    cout << "Server Client Interaction\n";
    string username;
    while (true)
    {
        char *data = readClientCommand(csoc);
        if (strncmp(data, "LGIN", 4) == 0)
        {
            // TODO: Implement Login
            string user = strtok(data + 4, " ");
            string password = strtok(data + 20, " ");
            if (users.find(user) != users.end() && users[user].first == password && users[user].second == false)
            {
                users[user].second = true;
                username = user;
                char *response = "0008GOOD";
                send(csoc, response, 8, 0);
            }
            else if (users.find(user) == users.end())
            {
                users[user] = {password, true};
                username = user;
                char *response = "0008GOOD";
                send(csoc, response, 8, 0);
            }
            else
            {
                char *response = "0008ERRM";
                send(csoc, response, 8, 0);
            }
        }
        else if (strncmp(data, "LOUT", 4) == 0)
        {
            // TODO: Implement Logout
            string user = strtok(data + 4, " ");
            if (users.find(user) != users.end() && users[user].second == true && user == username)
            {
                users[user].second = false;
                char *response = "0008GOOD";
                send(csoc, response, 8, 0);
            }
            else
            {
                char *response = "0008ERRM";
                send(csoc, response, 8, 0);
            }
        }
        else if (strncmp(data, "POST", 4) == 0)
        {
            // TODO: Implement Post

            // Send Response
            char *response = "0008GOOD";
            send(csoc, response, 8, 0);
        }
        else if (strncmp(data, "GETM", 4) == 0)
        {
            // TODO: Implement Get Messages

            // Send Response
            char *response = "0008LIST";
            send(csoc, response, 8, 0);
        }
        else if (strncmp(data, "EXIT", 4) == 0)
        {
            // Send Response
            char *response = "0008GOOD";
            send(csoc, response, 8, 0);
            break;
        }
        else
        {
            cout << "Invalid Command " << data << endl;
        }
    }
}