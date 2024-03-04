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

// Object Definitions
struct NBResponse
{
    int size;
    char *type;
    char *data;

    NBResponse(int &csoc)
    {
        char buf[5];
        buf[4] = '\0';
        recvLoop(csoc, buf, 4);
        this->size = atoi(buf);
        recvLoop(csoc, buf, 4);
        this->type = buf;
        if (size > 0)
        {
            char *data = new char[this->size];
            recvLoop(csoc, data, this->size);
            this->data = data;
            // cout << "Received data: " << this->data << endl;
        }
    }
};

// Function Definitions
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

void errorMessagePrint(int errCode)
{
    switch (errCode)
    {
    case 1:
        cout << "Wrong username or password\n";
        break;
    case 2:
        cout << "Logout failed\n";
        break;
    case 3:
        cout << "Post failed\n";
        break;
    case 4:
        cout << "Get Messages failed\n";
        break;
    case 5:
        cout << "Exit failed\n";
        break;
    case 7:
        cout << "Bid failed\n";
        break;
    case 8:
        cout << "Your bid is not the highest\n";
        break;
    default:
        cout << "Error\n";
        break;
    }
}

string messageEncode(string msg)
{
    string op = to_string(msg.size());
    while (op.size() < 4)
    {
        op = "0" + op;
    }
    op += "POST";
    op += msg;
    return op;
}

void printMessages(NBResponse *msg)
{
    cout << "Messages:\n";
    // data format: timestamp;name,timestamp,ttl,message)=msg-end=(

    string data = string(msg->data);
    string delimiter = ")=msg-end=(";
    size_t pos = 0;
    string token;
    while ((pos = data.find(delimiter)) != string::npos)
    {
        // format: timestamp;name,timestamp,ttl,message
        auto stt = data.find(";") + 1;
        cout << "From: " << data.substr(stt, data.find(",", stt) - stt) << endl;
        stt = data.find(",", stt) + 1;
        cout << "Timestamp: " << data.substr(stt, data.find(",", stt) - stt) << endl;
        stt = data.find(",", stt) + 1;
        cout << "TTL: " << data.substr(stt, data.find(",", stt) - stt) << endl;
        stt = data.find(",", stt) + 1;
        cout << "Message:\n"
             << data.substr(stt, pos - stt) << endl
             << endl;
        // token = data.substr(0, pos);
        //  cout << token << endl;
        data.erase(0, pos + delimiter.length());
    }
}

void printItems(NBResponse *msg)
{
    cout << "Items:\n";
    // data format: timestamp;name,timestamp,ttl,message)=msg-end=(

    string data = string(msg->data);
    string delimiter = ")=msg-end=(";
    size_t pos = 0;
    string token;
    while ((pos = data.find(delimiter)) != string::npos)
    {
        // format: timestamp;name,timestamp,ttl,message
        auto stt = data.find(";") + 1;
        cout << "Item: " << data.substr(stt, data.find(",", stt) - stt) << " ";
        stt = data.find(",", stt) + 1;
        cout << "(" << data.substr(stt, data.find(",", stt) - stt) << ")" << endl;
        stt = data.find(",", stt) + 1;
        cout << "From: " << data.substr(stt, data.find(",", stt) - stt) << endl;
        stt = data.find(",", stt) + 1;
        cout << "Timestamp: " << data.substr(stt, data.find(",", stt) - stt) << endl;
        stt = data.find(",", stt) + 1;
        cout << "TTL: " << data.substr(stt, data.find(",", stt) - stt) << endl;
        stt = data.find(",", stt) + 1;
        cout << "Price: " << data.substr(stt, data.find(",", stt) - stt) << endl;
        stt = data.find(",", stt) + 1;
        string bidder = data.substr(stt, data.find(",", stt) - stt);
        if (bidder == "NULL")
        {
            cout << "No bids\n";
            stt = data.find(",", stt) + 1;
        }
        else
        {
            stt = data.find(",", stt) + 1;
            cout << "Highest Bid: " << bidder << ", " << data.substr(stt, data.find(",", stt) - stt) << endl;
        }
        stt = data.find(",", stt) + 1;
        cout << "Description:\n"
             << data.substr(stt, pos - stt) << endl
             << endl;
        // token = data.substr(0, pos);
        //  cout << token << endl;
        data.erase(0, pos + delimiter.length());
    }
}

// Client Functions
void login(int &index, string &command, int &csoc, bool &loggedIn, string &username)
{
    if (loggedIn)
    {
        cout << "Already logged in\n";
        return;
    }
    char appmsg[40] = "0032LGIN";
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
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "GOOD", 4) == 0)
    {
        cout << "Login successful\n";
        loggedIn = true;
        username = user;
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Login error\n";
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
    char appmsg[24] = "0016LOUT";
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
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "GOOD", 4) == 0)
    {
        cout << "Logout successful\n";
        loggedIn = false;
        username = "";
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Logout error\n";
    }
}

void post(string &command, int &csoc, bool &loggedIn)
{
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }
    // Post send
    string msg = command.substr(5);
    string op = messageEncode(msg);
    char appmsg[op.length()];
    strcpy(appmsg, op.c_str());
    send(csoc, appmsg, op.length(), 0);

    // Post response
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "GOOD", 4) == 0)
    {
        cout << "Post successful\n";
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Post error\n";
    }
}

void getMessages(int &csoc, bool &loggedIn)
{
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }
    // Get Messages send
    char *appmsg = "0000GETM";
    send(csoc, appmsg, 8, 0);

    // Get Messages response
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "LIST", 4) == 0)
    {
        printMessages(response);
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Get Messages failed\n";
    }
}

void postItem(string &command, int &csoc, bool &loggedIn) // TODO: Error handling
{
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }
    // Post Item send
    string name = command.substr(6);
    cout << "Enter description:\n";
    string description;
    cout << "> ";
    getline(cin, description);
    cout << "Enter price:\n";
    string price;
    cout << "> ";
    getline(cin, price);
    string op = to_string(name.size() + description.size() + price.size() + 2);
    while (op.size() < 4)
    {
        op = "0" + op;
    }
    op += "PSTI";
    op += name + ";" + description + ";" + price;
    char appmsg[op.length()];
    strcpy(appmsg, op.c_str());
    send(csoc, appmsg, op.length(), 0);

    // Post Item response
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "GOOD", 4) == 0)
    {
        cout << "Item post successful\n";
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Item post failed\n";
    }
}

void getItems(int &csoc, bool &loggedIn)
{
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }
    // Get Items send
    char *appmsg = "0000GETI";
    send(csoc, appmsg, 8, 0);

    // Get Items response
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "LIST", 4) == 0)
    {
        printItems(response);
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Item retrieval failed\n";
    }
}

void bid(string &command, int &csoc, bool &loggedIn)
{
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }
    // Bid send
    string line = command.substr(4);
    string item = line.substr(0, line.find(" "));
    string price = line.substr(line.find(" ") + 1);
    item = item + ";" + price + ";";
    string op = to_string(item.size());
    while (op.size() < 4)
    {
        op = "0" + op;
    }
    op += "BIDD";
    op += item;
    char appmsg[op.length()];
    strcpy(appmsg, op.c_str());
    send(csoc, appmsg, op.length(), 0);

    // Bid response
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "GOOD", 4) == 0)
    {
        cout << "Bid successful\n";
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Bid failed\n";
    }
}

void exitProg(int &csoc)
{
    // Exit send
    char *appmsg = "0000EXIT";
    send(csoc, appmsg, 8, 0);

    // Exit response
    NBResponse *response = new NBResponse(csoc);
    if (strncmp(response->type, "GOOD", 4) == 0)
    {
        cout << "Exit successful\n";
    }
    else if (strncmp(response->type, "ERRM", 4) == 0)
    {
        errorMessagePrint(response->data[0] - '0');
    }
    else
    {
        cout << "Exit failed\n";
    }
}

// Client Interface
void clientInterface(int csoc)
{
    cout << "Client Interface\n";
    string username;
    bool loggedIn = false;
    while (true)
    {
        cout << "> ";
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
        else if (tokens[0] == "postm") // <sz>POST<message>
        {
            post(command, csoc, loggedIn);
        }
        else if (tokens[0] == "getm") // 0000GETM
        {
            getMessages(csoc, loggedIn);
        }
        else if (tokens[0] == "posti") // <sz>PSTI<item>
        {
            postItem(command, csoc, loggedIn);
        }
        else if (tokens[0] == "geti") // 0000GETI
        {
            getItems(csoc, loggedIn);
        }
        else if (tokens[0] == "bid") // <sz>BIDD<iid><price>
        {
            bid(command, csoc, loggedIn);
        }
        else if (tokens[0] == "exit") // 0008EXIT
        {
            exitProg(csoc);
            break;
        }
        else
        {
            cout << "Invalid command. To exit, call \"exit\".\n";
        }
    }
}