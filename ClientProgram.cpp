#include "ClientProgram.h"
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

char *readServerResponse(int &csoc)
{
    char buf[4];
    recvLoop(csoc, buf, 4);
    int size = atoi(buf);
    size -= 4;
    char *data = new char[size];
    recvLoop(csoc, data, size);
    // cout << "Server Response: " << data << endl;

    return data;
}

void errorMessagePrint() // TODO:
{
}

void login(int &index, string &command, int &csoc, bool &loggedIn, string &username)
{
    if (loggedIn)
    {
        cout << "Already logged in\n";
        return;
    }
    char appmsg[40] = "0040LGIN";
    string user;
    for (int i = 8; i < 24; i++)
    {
        if (index < command.size() && command[index] != ' ')
        {
            appmsg[i] = command[index];
            user += command[index];
            index++;
        }
        else
        {
            appmsg[i] = ' ';
        }
    }
    if (command[index] != ' ')
    {
        cout << "Username error\n";
        return;
    }
    index++;
    bool password = false;
    for (int i = 24; i < 40; i++)
    {
        if (index < command.size() && command[index] != ' ')
        {
            appmsg[i] = command[index];
            index++;
            password = true;
        }
        else
        {
            appmsg[i] = ' ';
        }
    }
    if (index < command.size() && command[index] != ' ' || !password)
    {
        cout << "Password error\n";
        return;
    }
    send(csoc, appmsg, 40, 0);

    // Login response
    char *response = readServerResponse(csoc);
    if (strncmp(response, "GOOD", 4) == 0)
    {
        cout << "Login successful\n";
        loggedIn = true;
        username = user;
    }
    else if (strncmp(response, "ERRM", 4) == 0)
    {
        cout << "Login failed with error message\n";
    }
    else
    {
        cout << "Login failed\n";
    }
}

void logout(int &csoc, bool &loggedIn, string &username)
{
    // Logout send
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }
    char appmsg[24] = "0024LOUT";
    int j = 0;
    for (int i = 8; i < 24; i++)
    {
        if (j < username.size())
        {
            appmsg[i] = username[j];
            j++;
        }
        else
        {
            appmsg[i] = ' ';
        }
    }
    send(csoc, appmsg, 24, 0);

    // Logout response
    char *response = readServerResponse(csoc);
    if (strncmp(response, "GOOD", 4) == 0)
    {
        cout << "Logout successful\n";
        loggedIn = false;
        username = "";
    }
    else if (strncmp(response, "ERRM", 4) == 0)
    {
        cout << "Logout failed with error message\n";
    }
    else
    {
        cout << "Logout failed\n";
    }
}

void exitProg(int &csoc)
{
    // Exit send
    char *appmsg = "0008EXIT";
    send(csoc, appmsg, 8, 0);

    // Exit response
    char *response = readServerResponse(csoc);
    if (strncmp(response, "GOOD", 4) == 0)
    {
        cout << "Exit successful\n";
    }
    else if (strncmp(response, "ERRM", 4) == 0)
    {
        cout << "Exit failed with error message\n";
    }
    else
    {
        cout << "Exit failed\n";
    }
}

void clientInterface(int csoc)
{
    cout << "Client Interface\n";
    string username;
    bool loggedIn = false;
    while (true)
    {
        string command;
        vector<string> tokens;
        getline(cin, command);
        int index = 0;

        string token;
        while (index < command.size() && command[index] != ' ')
        {
            token += command[index];
            index++;
        }
        tokens.push_back(token);
        index++;

        if (tokens[0] == "login") // 0040LGIN<username><password>
        {
            login(index, command, csoc, loggedIn, username);
        }
        else if (tokens[0] == "logout") // 0024LOUT<username>
        {
            logout(csoc, loggedIn, username);
        }
        else if (tokens[0] == "post") // TODO:
        {
            // Post send
            char appmsg[8] = "0008POS";
            appmsg[7] = 'T';
            send(csoc, appmsg, 8, 0);

            // Post response
            char *response = readServerResponse(csoc);
            if (strncmp(response, "GOOD", 4) == 0)
            {
                cout << "Post successful\n";
            }
            else if (strncmp(response, "ERRM", 4) == 0)
            {
                cout << "Post failed with error message\n";
            }
            else
            {
                cout << "Post failed\n";
            }
        }
        else if (tokens[0] == "getm") // TODO:
        {
            // Get Messages send
            char appmsg[8] = "0008GET";
            appmsg[7] = 'M';
            send(csoc, appmsg, 8, 0);

            // Get Messages response
            char *response = readServerResponse(csoc);
            if (strncmp(response, "LIST", 4) == 0)
            {
                cout << "Get Messages successful\n";
            }
            else if (strncmp(response, "ERRM", 4) == 0)
            {
                cout << "Get Messages failed with error message\n";
            }
            else
            {
                cout << "Get Messages failed\n";
            }
        }
        else if (tokens[0] == "exit") // 0008EXIT
        {
            exitProg(csoc);
            break;
        }
        else
        {
            cout << "Invalid command. To exit, call \"exit.\"\n";
        }
    }
}