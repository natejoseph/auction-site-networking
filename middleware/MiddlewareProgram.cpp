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
// TODO: TIME FUNCTIONALITY, GETM (NULL)

// Object Definitions
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

struct Post
{
    User *from;
    int timestamp;
    int ttl;
    string data;

    // optional
    User *to;
    bool visible;

    Post(User *from, int timestamp, int ttl, string data)
    {
        this->from = from;
        this->timestamp = timestamp;
        this->ttl = ttl;
        this->data = data;
        this->to = NULL;
        this->visible = true;
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

struct Bid
{
    User *bidder;
    int amount;
    int timestamp;

    Bid(User *bidder, int amount, int timestamp)
    {
        this->bidder = bidder;
        this->amount = amount;
        this->timestamp = timestamp;
    }
};

struct Item
{
    User *seller;
    string name;
    string description;
    int timestamp;
    int ttl;
    int price;
    int iid;

    // optional
    Bid *bid;
    bool visible;

    Item(User *seller, string name, string description, int price, int timestamp, int ttl, int iid)
    {
        this->seller = seller;
        this->name = name;
        this->description = description;
        this->price = price;
        this->timestamp = timestamp;
        this->ttl = ttl;
        this->iid = iid;
        this->bid = NULL;
        this->visible = true;
    }

    bool setBid(Bid *bid)
    {
        if (this->bid == NULL || bid->amount > this->bid->amount)
        {
            this->bid = bid;
            return true;
        }
        return false;
    }

    Bid *getBid()
    {
        return this->bid;
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
        cout << "Item: " << this->name << endl;
        cout << "From: " << this->seller->getUsername() << endl;
        cout << "Timestamp: " << this->timestamp << endl;
        cout << "TTL: " << this->ttl << endl;
        cout << "Price: " << this->price << endl;
        cout << "Description:\n"
             << this->description << endl;
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

// Global Variables
unordered_map<string, User *> users;
map<string, Post *> posts;
map<int, Item *> items;
unordered_map<int, User *> uids;

// Function Definitions
string messageEncode(NBResponse *msg)
{
    string op = to_string(msg->size);
    while (op.size() < 4)
    {
        op = "0" + op;
    }
    op += "LIST";
    string data = string(msg->data);
    string response = op + data;
    return response;
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
    char buf[5];
    buf[4] = '\0';
    recvLoop(csoc, buf, 4);
    int size = atoi(buf);
    char *data = new char[size + 4];
    recvLoop(csoc, data, size + 4);
    cout << "Client Command: " << data << endl;

    return new NBMessage(size, data);
}

void hardCodeUsers()
{
    users["nate"] = new User("nate", "joseph", 1);
    uids[1] = users["nate"];
    users["nigel"] = new User("nigel", "john", 2);
    uids[2] = users["nigel"];
    users["admin"] = new User("admin", "123", 3);
    uids[3] = users["admin"];
}

NBResponse *returnMessages(string data)
{
    stringstream ss;
    for (auto it = posts.begin(); it != posts.end(); it++)
    {
        if (it->second->checkActive())
        {
            ss << it->first << ";" << it->second->from->getUsername() << ","
               << it->second->timestamp << "," << it->second->ttl << "," << it->second->data << ")=msg-end=(";
        }
    }
    data = ss.str();
    return new NBResponse(data.size(), (char *)data.c_str());
}

NBResponse *returnItems(string data)
{
    stringstream ss;
    for (auto it = items.begin(); it != items.end(); it++)
    {
        if (it->second->checkActive())
        {
            ss << it->first << ";" << it->second->name << "," << it->second->iid << "," << it->second->seller->getUsername() << ","
               << it->second->timestamp << "," << it->second->ttl << "," << it->second->price << ",";
            if (it->second->getBid() != NULL)
            {
                ss << it->second->getBid()->bidder->getUsername() << "," << it->second->getBid()->amount << ",";
            }
            else
            {
                ss << "NULL,0,";
            }
            ss << it->second->description << ")=msg-end=(";
        }
    }
    data = ss.str();
    return new NBResponse(data.size(), (char *)data.c_str());
}

// Command Functions
void login(NBMessage *msg, int &csoc, int &uid)
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
        char *response = "0001ERRM1";
        send(csoc, response, 9, 0);
    }
}

void logout(NBMessage *msg, int &csoc, int &uid)
{
    string user = strtok(msg->data + 4, " ");
    if (users.find(user) != users.end() && uid == users[user]->getUID())
    {
        uid = -1;
        char *response = "0000GOOD";
        send(csoc, response, 8, 0);
    }
    else
    {
        char *response = "0001ERRM2";
        send(csoc, response, 9, 0);
    }
}

void post(NBMessage *msg, int &csoc, int &uid, int &messageIndex)
{
    if (uid == -1)
    {
        char *response = "0001ERRM3";
        send(csoc, response, 9, 0);
        return;
    }

    char data[msg->size + 1]; // = msg->data+4;
    strncpy(data, msg->data + 4, msg->size);
    data[msg->size] = '\0';
    int t = messageIndex++;
    Post *post = new Post(uids[uid], t, 10, data);
    posts[to_string(t)] = post;
    char *response = "0000GOOD";
    send(csoc, response, 8, 0);

    post->display();
}

void getMessages(int &csoc, int &uid)
{
    if (uid == -1)
    {
        char *response = "0001ERRM4";
        send(csoc, response, 9, 0);
        return;
    }

    // Send Response
    string data;
    string resp = messageEncode(returnMessages(data));
    char *response = (char *)resp.c_str();
    send(csoc, response, strlen(response), 0);
}

void postItem(NBMessage *msg, int &csoc, int &uid, int &itemIndex)
{
    if (uid == -1)
    {
        char *response = "0001ERRM5";
        send(csoc, response, 9, 0);
        return;
    }

    char data[msg->size + 1]; // = msg->data+4;
    strncpy(data, msg->data + 4, msg->size);
    data[msg->size] = '\0';
    string name = strtok(data, ";");
    string description = strtok(NULL, ";");
    int price = atoi(strtok(NULL, ";"));
    int t = itemIndex++;
    Item *item = new Item(uids[uid], name, description, price, t, 10, t);
    items[t] = item;
    char *response = "0000GOOD";
    send(csoc, response, 8, 0);

    item->display();
}

void getItems(int &csoc, int &uid)
{
    if (uid == -1)
    {
        char *response = "0001ERRM6";
        send(csoc, response, 9, 0);
        return;
    }

    // Send Response
    string data;
    string resp = messageEncode(returnItems(data));
    char *response = (char *)resp.c_str();
    send(csoc, response, strlen(response), 0);
}

void bid(NBMessage *msg, int &csoc, int &uid)
{
    if (uid == -1)
    {
        char *response = "0001ERRM7";
        send(csoc, response, 9, 0);
        return;
    }

    char data[msg->size + 1]; // = msg->data+4;
    strncpy(data, msg->data + 4, msg->size);
    data[msg->size] = '\0';
    int item = atoi(strtok(data, ";"));
    int amount = atoi(strtok(NULL, ";"));
    int t = time(0);
    Bid *bid = new Bid(uids[uid], amount, t);
    if (items[item]->setBid(bid))
    {
        char *response = "0000GOOD";
        send(csoc, response, 8, 0);
    }
    else
    {
        char *response = "0001ERRM8";
        send(csoc, response, 9, 0);
    }
}

void exitProg(int &csoc, int &uid)
{
    // Send Response
    char *response = "0000GOOD";
    uid = -1;
    send(csoc, response, 8, 0);
}

// Server Client Interaction
void serverClientInteraction(int csoc)
{
    cout << "Server Client Interaction\n";
    int uid = -1;
    hardCodeUsers();
    int messageIndex = 1;
    int itemIndex = 1;
    while (true)
    {
        NBMessage *msg = readClient(csoc);
        if (strncmp(msg->data, "LGIN", 4) == 0)
        {
            login(msg, csoc, uid);
        }
        else if (strncmp(msg->data, "LOUT", 4) == 0)
        {
            logout(msg, csoc, uid);
        }
        else if (strncmp(msg->data, "POST", 4) == 0)
        {
            post(msg, csoc, uid, messageIndex);
        }
        else if (strncmp(msg->data, "GETM", 4) == 0)
        {
            getMessages(csoc, uid);
        }
        else if (strncmp(msg->data, "PSTI", 4) == 0)
        {
            postItem(msg, csoc, uid, itemIndex);
        }
        else if (strncmp(msg->data, "GETI", 4) == 0)
        {
            getItems(csoc, uid);
        }
        else if (strncmp(msg->data, "BIDD", 4) == 0)
        {
            bid(msg, csoc, uid);
        }
        else if (strncmp(msg->data, "EXIT", 4) == 0)
        {
            exitProg(csoc, uid);
            break;
        }
        else
        {
            cout << "Invalid Command " << msg->data << endl;
        }
    }
}