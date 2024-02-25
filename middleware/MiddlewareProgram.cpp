#include "MiddlewareProgram.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>

using namespace std;
// TODO: REWORK SIZE, TIME FUNCTIONALITY

class User
{
    string username;
    string password;
    int uid;

public:
    User(string username, string password, int uid)
    {
        this->username = username;
        this->password = password;
        this->uid = uid;
    }

    bool checkPassword(string password)
    {
        return this->password == password;
    }

    string getUsername()
    {
        return this->username;
    }

    int getUID()
    {
        return this->uid;
    }
};

struct Message
{
    User *from;
    int timestamp;
    int ttl;
    string data;

    // optional
    User *to;
    bool visible;

    Message(User *from, int timestamp, int ttl, string data)
    {
        this->from = from;
        this->timestamp = timestamp;
        this->ttl = ttl;
        this->data = data;
    }

    void setTo(User *to)
    {
        this->to = to;
    }

    void setVisible(bool visible)
    {
        this->visible = visible;
    }

    bool checkActive()
    {
        return this->visible;
    }

    void display()
    {
        cout << "From: " << this->from->getUsername() << endl;
        cout << "Timestamp: " << this->timestamp << endl;
        cout << "TTL: " << this->ttl << endl;
        cout << "Message:\n"
             << this->data << endl;
    }
};

struct NBMessage
{
    int size;
    char *data;

    NBMessage(int size, char *data)
    {
        this->size = size;
        this->data = data;
    }
};

struct NBResponse
{
    int size;
    char *data;

    NBResponse(int size, char *data)
    {
        this->size = size;
        this->data = data;
    }
};

string messageEncode(NBResponse *msg)
{
    string op = to_string(msg->size + 4);
    while (op.size() < 4)
    {
        op = "0" + op;
    }
    op += "LIST";
    string data = string(msg->data);
    return op + data;
}

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

NBMessage *readClient(int csoc)
{
    char buf[4];
    recvLoop(csoc, buf, 4);
    int size = atoi(buf);
    char *data = new char[size + 4];
    recvLoop(csoc, data, size + 4);
    cout << "Client Command: " << data << endl;

    return new NBMessage(size, data);
}

unordered_map<string, User *> users;
map<string, Message *> msgs;
unordered_map<int, User *> uids;

void hardCodeUsers()
{
    users["nate"] = new User("nate", "joseph", 1);
    uids[1] = users["nate"];
    users["nigel"] = new User("nigel", "john", 2);
    uids[2] = users["nigel"];
}

NBResponse *returnMessages()
{
    stringstream ss;
    for (auto it = msgs.begin(); it != msgs.end(); it++)
    {
        if (it->second->checkActive())
        {
            ss << "(" << it->first << ";" << it->second->from->getUsername() << "," << it->second->timestamp << "," << it->second->ttl << "," << it->second->data << ")";
        }
    }
    string data = ss.str();
    return new NBResponse(data.size(), (char *)data.c_str());
}

void serverClientInteraction(int csoc)
{
    cout << "Server Client Interaction\n";
    int uid = -1;
    hardCodeUsers();
    int messageIndex = 1;
    while (true)
    {
        NBMessage *msg = readClient(csoc);
        if (strncmp(msg->data, "LGIN", 4) == 0)
        {
            string user = strtok(msg->data + 4, " ");
            string password = strtok(msg->data + 20, " ");
            if (users.find(user) != users.end() && users[user]->checkPassword(password) && uid == -1)
            {
                uid = users[user]->getUID();
                char *response = "0000GOOD";
                send(csoc, response, 8, 0);
            }
            else
            {
                char *response = "0000ERRM";
                send(csoc, response, 8, 0);
            }
        }
        else if (strncmp(msg->data, "LOUT", 4) == 0)
        {
            string user = strtok(msg->data + 4, " ");
            if (users.find(user) != users.end() && uid == users[user]->getUID())
            {
                uid = -1;
                char *response = "0008GOOD";
                send(csoc, response, 8, 0);
            }
            else
            {
                char *response = "0008ERRM";
                send(csoc, response, 8, 0);
            }
        }
        else if (strncmp(msg->data, "POST", 4) == 0)
        {
            // string message
            // Send Response
            if (uid == -1)
            {
                char *response = "0008ERRM";
                send(csoc, response, 8, 0);
                continue;
            }

            cout << "here\n";

            char data[msg->size - 3]; // = msg->data+4;
            strncpy(data, msg->data + 4, msg->size - 4);
            data[msg->size - 4] = '\0';
            int t = messageIndex++;
            Message *message = new Message(uids[uid], t, 10, data);
            msgs[to_string(t)] = message;
            char *response = "0008GOOD";
            send(csoc, response, 8, 0);

            message->display();
        }
        else if (strncmp(msg->data, "GETM", 4) == 0)
        {
            // TODO: Implement Get Messages

            // Send Response
            char *response = (char *)messageEncode(returnMessages()).c_str();
            send(csoc, response, strlen(response), 0);
        }
        else if (strncmp(msg->data, "EXIT", 4) == 0)
        {
            // Send Response
            char *response = "0008GOOD";
            uid = -1;
            send(csoc, response, 8, 0);
            break;
        }
        else
        {
            cout << "Invalid Command " << msg->data << endl;
        }
        break;
    }
}