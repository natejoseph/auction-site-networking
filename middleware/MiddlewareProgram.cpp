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
// TODO: TIME FUNCTIONALITY
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

struct ServerRequest
{
    int size;
    char *data;

    ServerRequest(int size, char *data)
    {
        this->size = size;
        this->data = data;
    }

    void sendReq(int scsoc) // <size><data>
    {
        string op = to_string(this->size);
        while (op.size() < 4)
        {
            op = "0" + op;
        }
        string data = string(this->data);
        string res = op + data;

        char *response = (char *)res.c_str();
        send(scsoc, response, strlen(response), 0);
    }
};

struct ServerResponse
{
    int size;
    char *data;

    ServerResponse(int &csoc)
    {
        char buf[5];
        buf[4] = '\0';
        recvLoop(csoc, buf, 4);
        this->size = atoi(buf);
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

NBResponse *returnMessages(string data, int &ssoc, ServerResponse *res)
{
    stringstream ss;
    string info = string(res->data);
    while (true)
    {
        string uid = "uid::" + info.substr(0, info.find(";"));
        ServerRequest *req = new ServerRequest(uid.length(), (char *)uid.c_str());
        req->sendReq(ssoc);
        cout << "Sent Request: " << req->data << endl;
        ServerResponse *res2 = new ServerResponse(ssoc);
        cout << "Received Response: " << res2->data << endl;
        string username = string(res2->data);

        ss << "0;" << username << ",";
        auto it = info.find(";") + 1;
        ss << info.substr(it, info.find(";", it) - it) << ",";
        it = info.find(";", it) + 1;
        ss << info.substr(it, info.find("::", it) - it) << ")=msg-end=(";
        info.erase(0, info.find("::") + 2);
        if (info.find("::") == string::npos)
        {
            break;
        }
    }
    data = ss.str();
    return new NBResponse(data.size(), (char *)data.c_str());
}

NBResponse *returnItems(string data, int &ssoc, ServerResponse *res)
{
    stringstream ss;
    string info = string(res->data);
    while (true)
    {
        string itemNum = info.substr(0, info.find(";"));
        auto it = info.find(";") + 1;
        string uid = "uid::" + info.substr(it, info.find(";", it) - it);
        ServerRequest *req = new ServerRequest(uid.length(), (char *)uid.c_str());
        req->sendReq(ssoc);
        cout << "Sent Request: " << req->data << endl;
        ServerResponse *res2 = new ServerResponse(ssoc);
        cout << "Received Response: " << res2->data << endl;
        string username = string(res2->data);
        it = info.find(";", it) + 1;

        ss << "0;";
        ss << info.substr(it, info.find(";", it) - it) << ","; // name
        ss << itemNum << ",";                                  // iid
        ss << username << ",";                                 // seller
        it = info.find(";", it) + 1;
        ss << info.substr(it, info.find(";", it) - it) << ","; // timestamp
        it = info.find(";", it) + 1;
        ss << info.substr(it, info.find(";", it) - it) << ","; // price
        it = info.find(";", it) + 1;

        string key = "get::bidd::" + itemNum;
        ServerRequest *req3 = new ServerRequest(key.length(), (char *)key.c_str());
        req3->sendReq(ssoc);
        cout << "Sent Request: " << req3->data << endl;
        ServerResponse *res3 = new ServerResponse(ssoc);
        cout << "Received Response: " << res3->data << endl;
        string resData3 = string(res3->data);
        if (resData3 == "NULL")
        {
            ss << "NULL,0,"; // bid
        }
        else
        {
            string info = string(resData3);
            auto it = info.find(";");
            string prevBidder = info.substr(0, it);
            string user = "uid::" + prevBidder;
            ServerRequest *req = new ServerRequest(user.length(), (char *)user.c_str());
            req->sendReq(ssoc);
            cout << "Sent Request: " << req->data << endl;
            ServerResponse *res = new ServerResponse(ssoc);
            cout << "Received Response: " << res->data << endl;
            string bidder = string(res->data);
            it = info.find(";", it) + 1;
            string prevAmount = info.substr(it, info.find("::") - it);
            ss << bidder << ",";     // bid
            ss << prevAmount << ","; // amount
        }
        ss << info.substr(it, info.find("::") - it) << ")=msg-end=("; // description
        info.erase(0, info.find("::") + 2);
        if (info.find("::") == string::npos)
        {
            break;
        }
    }
    data = ss.str();
    return new NBResponse(data.size(), (char *)data.c_str());
}

// Command Functions
void login(NBMessage *msg, int &csoc, int &ssoc, int &uid)
{
    string user = strtok(msg->data + 4, " ");
    string password = strtok(msg->data + 20, " ");

    user = "user::" + user;
    char *u = (char *)user.c_str();
    ServerRequest *req = new ServerRequest(strlen(u), u);
    req->sendReq(ssoc);
    cout << "Sent Request: " << req->data << endl;
    ServerResponse *res = new ServerResponse(ssoc);
    cout << "Received Response: " << res->data << endl;
    string data = string(res->data);
    if (data == "NULL")
    {
        char *response = "0002ERRM12";
        send(csoc, response, 10, 0);
        return;
    }
    string pw = strtok(res->data, ";");
    string id = strtok(NULL, ";");
    if (pw == password && uid == -1)
    {
        uid = stoi(id);
        char *response = "0000GOOD";
        send(csoc, response, 8, 0);
    }
    else
    {
        char *response = "0001ERRM1";
        send(csoc, response, 9, 0);
    }
}

void logout(NBMessage *msg, int &csoc, int &ssoc, int &uid)
{
    string user = strtok(msg->data + 4, " ");
    user = "user::" + user;
    ServerRequest *req = new ServerRequest(user.length(), (char *)user.c_str());
    req->sendReq(ssoc);
    cout << "Sent Request: " << req->data << endl;
    ServerResponse *res = new ServerResponse(ssoc);
    cout << "Received Response: " << res->data << endl;
    string data = string(res->data);
    if (data == "NULL")
    {
        char *response = "0001ERRM2";
        send(csoc, response, 10, 0);
        return;
    }

    uid = -1;
    char *response = "0000GOOD";
    send(csoc, response, 8, 0);
}

void post(NBMessage *msg, int &csoc, int &ssoc, int &uid)
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
    int ttl = 30;

    string key = "send::mess::" + to_string(uid);
    string value = to_string(ttl) + ";" + to_string(uid) + ";" + data;
    ServerRequest *req = new ServerRequest(key.length(), (char *)key.c_str());
    req->sendReq(ssoc);
    cout << "Sent Request: " << req->data << endl;
    ServerResponse *res = new ServerResponse(ssoc);
    cout << "Received Response: " << res->data << endl;
    string resData = string(res->data);
    if (resData != "KEY")
    {
        char *response = "0002ERRM13";
        send(csoc, response, 10, 0);
        return;
    }

    ServerRequest *req2 = new ServerRequest(value.length(), (char *)value.c_str());
    req2->sendReq(ssoc);
    cout << "Sent Request: " << req2->data << endl;
    ServerResponse *res2 = new ServerResponse(ssoc);
    cout << "Received Response: " << res2->data << endl;
    string resData2 = string(res2->data);
    if (resData2 != "OK")
    {
        char *response = "0002ERRM14";
        send(csoc, response, 10, 0);
        return;
    }
    char *response = "0000GOOD";
    send(csoc, response, 8, 0);
}

void getMessages(int &csoc, int &ssoc, int &uid)
{
    if (uid == -1)
    {
        char *response = "0001ERRM4";
        send(csoc, response, 9, 0);
        return;
    }

    string serverData = "get::mess::";
    if (false) // inbox
    {
        serverData += to_string(uid);
    }
    else
    {
        serverData += "all";
    }
    ServerRequest *req = new ServerRequest(serverData.length(), (char *)serverData.c_str());
    req->sendReq(ssoc);
    cout << "Sent Request: " << req->data << endl;
    ServerResponse *res = new ServerResponse(ssoc);
    cout << "Received Response: " << res->data << endl;
    string resData = string(res->data);
    if (resData == "NULL")
    {
        char *response = "0001ERRM9";
        send(csoc, response, 9, 0);
        return;
    }
    // Send Response
    string data;
    string resp = messageEncode(returnMessages(data, ssoc, res));
    char *response = (char *)resp.c_str();
    send(csoc, response, strlen(response), 0);
}

void postItem(NBMessage *msg, int &csoc, int &ssoc, int &uid)
{
    if (uid == -1)
    {
        char *response = "0001ERRM5";
        send(csoc, response, 9, 0);
        return;
    }
    // Error Checking if parameters have been inputted correctly

    char data[msg->size + 1]; // = msg->data+4;
    strncpy(data, msg->data + 4, msg->size);
    data[msg->size] = '\0';
    int ttl = 20;

    string key = "send::item::" + to_string(uid);
    string value = to_string(ttl) + ";" + to_string(uid) + ";" + data;
    ServerRequest *req = new ServerRequest(key.length(), (char *)key.c_str());
    req->sendReq(ssoc);
    cout << "Sent Request: " << req->data << endl;
    ServerResponse *res = new ServerResponse(ssoc);
    cout << "Received Response: " << res->data << endl;
    string resData = string(res->data);
    if (resData != "KEY")
    {
        char *response = "0002ERRM13";
        send(csoc, response, 10, 0);
        return;
    }

    ServerRequest *req2 = new ServerRequest(value.length(), (char *)value.c_str());
    req2->sendReq(ssoc);
    cout << "Sent Request: " << req2->data << endl;
    ServerResponse *res2 = new ServerResponse(ssoc);
    cout << "Received Response: " << res2->data << endl;
    string resData2 = string(res2->data);
    if (resData2 != "OK")
    {
        char *response = "0002ERRM14";
        send(csoc, response, 10, 0);
        return;
    }

    /*string name = strtok(data, ";");
    string description = strtok(NULL, ";");
    int price = atoi(strtok(NULL, ";"));
    int t = itemIndex++;
    Item *item = new Item(uids[uid], name, description, price, t, 10, t);
    items[t] = item;*/
    char *response = "0000GOOD";
    send(csoc, response, 8, 0);

    // item->display();
}

void getItems(int &csoc, int &ssoc, int &uid)
{
    if (uid == -1)
    {
        char *response = "0001ERRM6";
        send(csoc, response, 9, 0);
        return;
    }

    string serverData = "get::item::all";
    ServerRequest *req = new ServerRequest(serverData.length(), (char *)serverData.c_str());
    req->sendReq(ssoc);
    cout << "Sent Request: " << req->data << endl;
    ServerResponse *res = new ServerResponse(ssoc);
    cout << "Received Response: " << res->data << endl;
    string resData = string(res->data);
    if (resData == "NULL")
    {
        char *response = "0002ERRM10";
        send(csoc, response, 10, 0);
        return;
    }
    // Send Response
    string data;
    string resp = messageEncode(returnItems(data, ssoc, res));
    char *response = (char *)resp.c_str();
    send(csoc, response, strlen(response), 0);
}

void bid(NBMessage *msg, int &csoc, int &ssoc, int &uid)
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
    string item = strtok(data, ";");
    int amount = atoi(strtok(NULL, ";"));

    string serverData = "get::item::" + item;
    ServerRequest *req = new ServerRequest(serverData.length(), (char *)serverData.c_str());
    req->sendReq(ssoc);
    cout << "Sent Request: " << req->data << endl;
    ServerResponse *res = new ServerResponse(ssoc);
    cout << "Received Response: " << res->data << endl;
    string resData = string(res->data);
    if (resData == "NULL")
    {
        char *response = "0002ERRM11";
        send(csoc, response, 10, 0);
        return;
    }
    // TODO: create a bid on the found item
    serverData = "get::bidd::" + item;
    ServerRequest *req2 = new ServerRequest(serverData.length(), (char *)serverData.c_str());
    req2->sendReq(ssoc);
    cout << "Sent Request: " << req2->data << endl;
    ServerResponse *res2 = new ServerResponse(ssoc);
    cout << "Received Response: " << res2->data << endl;
    string resData2 = string(res2->data);
    if (resData2 == "NULL")
    {
        // no previous bids
        int ttl = 25;
        string key = "send::bidd::" + item;
        string value = to_string(ttl) + ";" + to_string(uid) + ";" + to_string(amount);
        ServerRequest *req3 = new ServerRequest(key.length(), (char *)key.c_str());
        req3->sendReq(ssoc);
        cout << "Sent Request: " << req3->data << endl;
        ServerResponse *res3 = new ServerResponse(ssoc);
        cout << "Received Response: " << res3->data << endl;
        string resData3 = string(res3->data);
        if (resData3 != "KEY")
        {
            char *response = "0002ERRM13";
            send(csoc, response, 10, 0);
            return;
        }

        ServerRequest *req4 = new ServerRequest(value.length(), (char *)value.c_str());
        req4->sendReq(ssoc);
        cout << "Sent Request: " << req4->data << endl;
        ServerResponse *res4 = new ServerResponse(ssoc);
        cout << "Received Response: " << res4->data << endl;
        string resData4 = string(res4->data);
        if (resData4 != "OK")
        {
            char *response = "0002ERRM14";
            send(csoc, response, 10, 0);
            return;
        }
        char *response = "0000GOOD";
        send(csoc, response, 8, 0);
    }
    else
    {
        // compare bid to previous bid
        string info = string(res2->data);
        auto it = info.find(";");
        string prevBidder = info.substr(0, it);
        it = info.find(";", it) + 1;
        string prevAmount = info.substr(it, info.find("::") - it);
        if (amount > stoi(prevAmount))
        {
            int ttl = 10;
            string key = "send::bidd::" + item;
            string value = to_string(ttl) + ";" + to_string(uid) + ";" + to_string(amount);
            ServerRequest *req3 = new ServerRequest(key.length(), (char *)key.c_str());
            req3->sendReq(ssoc);
            cout << "Sent Request: " << req3->data << endl;
            ServerResponse *res3 = new ServerResponse(ssoc);
            cout << "Received Response: " << res3->data << endl;
            string resData3 = string(res3->data);
            if (resData3 != "KEY")
            {
                char *response = "0002ERRM13";
                send(csoc, response, 10, 0);
                return;
            }

            ServerRequest *req4 = new ServerRequest(value.length(), (char *)value.c_str());
            req4->sendReq(ssoc);
            cout << "Sent Request: " << req4->data << endl;
            ServerResponse *res4 = new ServerResponse(ssoc);
            cout << "Received Response: " << res4->data << endl;
            string resData4 = string(res4->data);
            if (resData4 != "OK")
            {
                char *response = "0002ERRM14";
                send(csoc, response, 10, 0);
                return;
            }
            char *response = "0000GOOD";
            send(csoc, response, 8, 0);
        }
        else
        {
            char *response = "0001ERRM8";
            send(csoc, response, 9, 0);
        }
    }
}

void exitProg(NBMessage *msg, int &csoc, int &uid)
{
    /*if (msg->size == 1)
    {
        char data[msg->size + 1]; // = msg->data+4;
        strncpy(data, msg->data + 4, msg->size);
        data[msg->size] = '\0';
        if (strncmp(data, "F", 1) == 0)
        {
            if (uid == 0)
            {
                // shutdown server
                string serverData = "exit::full";
                ServerRequest *req = new ServerRequest(serverData.length(), (char *)serverData.c_str());
                req->sendReq(csoc);
                cout << "Sent Request: " << req->data << endl;
                ServerResponse *res = new ServerResponse(csoc);
                cout << "Received Response: " << res->data << endl;
                string resData = string(res->data);
                if (resData == "EXIT")
                {
                    cout << "Server Shutdown\n";
                    char *response = "0000GOOD";
                    send(csoc, response, 8, 0);
                    return;
                }
                else
                {
                    cout << "Server Shutdown Error\n";
                    char *response = "0002ERRM16";
                    send(csoc, response, 10, 0);
                    return;
                }
            }
            else
            {
                char *response = "0002ERRM15";
                send(csoc, response, 10, 0);
                return;
            }
        }
    }*/
    // Send Response
    char *response = "0000GOOD";
    uid = -1;
    send(csoc, response, 8, 0);
}

// Middleware Client Interaction
void middlewareClientInteraction(int csoc, int ssoc)
{
    cout << "Server Client Interaction\n";
    int uid = -1;
    int messageIndex = 1;
    int itemIndex = 1;
    while (true)
    {
        NBMessage *msg = readClient(csoc);
        if (strncmp(msg->data, "LGIN", 4) == 0)
        {
            login(msg, csoc, ssoc, uid);
        }
        else if (strncmp(msg->data, "LOUT", 4) == 0)
        {
            logout(msg, csoc, ssoc, uid);
        }
        else if (strncmp(msg->data, "POST", 4) == 0)
        {
            post(msg, csoc, ssoc, uid);
        }
        else if (strncmp(msg->data, "GETM", 4) == 0)
        {
            getMessages(csoc, ssoc, uid);
        }
        else if (strncmp(msg->data, "PSTI", 4) == 0)
        {
            postItem(msg, csoc, ssoc, uid);
        }
        else if (strncmp(msg->data, "GETI", 4) == 0)
        {
            getItems(csoc, ssoc, uid);
        }
        else if (strncmp(msg->data, "BIDD", 4) == 0)
        {
            bid(msg, csoc, ssoc, uid);
        }
        else if (strncmp(msg->data, "EXIT", 4) == 0)
        {
            exitProg(msg, csoc, uid);
            break;
        }
        else
        {
            cout << "Invalid Command " << msg->data << endl;
        }
    }
}