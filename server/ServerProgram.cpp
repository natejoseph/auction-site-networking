#include "ServerProgram.h"
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

// Object Definitions
struct MWRequest
{
    int size;
    char *data;

    MWRequest(int &csoc)
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

struct MWResponse
{
    int size;
    char *data;

    MWResponse(char *data, int scsoc)
    {
        this->data = data;
        this->size = strlen(data);
        sendResp(scsoc);
    }

    void sendResp(int scsoc) // <size><data>
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

// Global Variables
map<string, string> serverData;

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

void hardCodeServerData()
{
    serverData["uid::0"] = "admin";
    serverData["user::admin"] = "123;0;";
    serverData["uid::1"] = "nate";
    serverData["user::nate"] = "joseph;1;";
    serverData["uid::2"] = "nigel";
    serverData["user::nigel"] = "john;2;";
}

// Server Middleware Interaction
void serverMiddlewareInteraction(int csoc)
{
    cout << "Server Middleware Interaction\n";
    hardCodeServerData();
    int inputNum = 0;
    int t = 1;
    while (true)
    {
        MWRequest *req = new MWRequest(csoc);
        cout << "Received data: " << req->data << endl;
        if (strncmp(req->data, "send::", 6) == 0) // key: <kind>::<uid>:;<inputNum>
        {                                         // value: <ttl>;<uid>;<value>
            string data = string(req->data);
            string key = data.substr(6, data.size());
            key += ":;" + to_string(++inputNum);
            cout << "Key: " << key << endl;
            MWResponse *res = new MWResponse((char *)"KEY", csoc);
            cout << "Sent data: " << res->data << endl;
            MWRequest *req = new MWRequest(csoc);
            string value = string(req->data);
            cout << "Value: " << req->data << endl;
            // turn ttl to ttd
            int ttl = stoi(value.substr(0, value.find(";")));
            ttl += t;
            value = to_string(ttl) + value.substr(value.find(";"), value.size());
            serverData[key] = value;
            MWResponse *res2 = new MWResponse((char *)"OK", csoc);
            cout << "Sent data: " << res2->data << endl;
        }
        else if (strncmp(req->data, "get::", 5) == 0) // request: <kind>::<uid>:;<inputNum>
        {
            string kind = string(req->data).substr(5, 4);
            kind = kind + "::";
            string key = string(req->data + 11);
            cout << "Kind: " << kind << endl;
            if (key == "all")
            {
                string data = "";
                for (auto it = serverData.begin(); it != serverData.end(); it++)
                {
                    if (kind == it->first.substr(0, 6))
                    {
                        if (kind == "item::")
                        {
                            data += it->first.substr(it->first.find(":;") + 2) + ";";
                        }
                        data += it->second.substr(it->second.find(";") + 1) + "::";
                    }
                }
                if (data == "")
                {
                    data = "NULL";
                }
                cout << "Data: " << data << endl;
                MWResponse *res = new MWResponse((char *)data.c_str(), csoc);
                cout << "Sent data: " << res->data << endl;
            }
            else if (kind == "mess::")
            {
                string data = "";
                kind += key + ":;";
                for (auto it = serverData.begin(); it != serverData.end(); it++)
                {
                    if (it->first.rfind(kind, 0) == 0)
                    {
                        data += it->second + "::";
                    }
                }
                if (data == "")
                {
                    data = "NULL";
                }
                MWResponse *res = new MWResponse((char *)data.c_str(), csoc);
                cout << "Sent data: " << res->data << endl;
            }
            else if (kind == "item::")
            {
                string data = "";
                for (auto it = serverData.begin(); it != serverData.end(); it++)
                {
                    if (kind == it->first.substr(0, 6) && key == it->first.substr(it->first.find(":;") + 2))
                    {
                        data += it->second.substr(it->second.find(";") + 1) + "::";
                        break;
                    }
                }
                if (data == "")
                {
                    data = "NULL";
                }
                MWResponse *res = new MWResponse((char *)data.c_str(), csoc);
                cout << "Sent data: " << res->data << endl;
            }
            else if (kind == "bidd::")
            {
                string data = "";
                int mostRecent = 0;
                for (auto it = serverData.begin(); it != serverData.end(); it++)
                {
                    if (kind == it->first.substr(0, 6) && key == it->first.substr(it->first.find("::") + 2, key.size()))
                    {
                        int temp = stoi(it->first.substr(it->first.find(":;") + 2));
                        if (temp > mostRecent)
                        {
                            data = it->second.substr(it->second.find(";") + 1) + "::";
                            mostRecent = temp;
                        }
                    }
                }
                if (data == "")
                {
                    data = "NULL";
                }
                MWResponse *res = new MWResponse((char *)data.c_str(), csoc);
                cout << "Sent data: " << res->data << endl;
            }
        }
        else if (strncmp(req->data, "exit::full", 10) == 0)
        {
            MWResponse *res = new MWResponse((char *)"EXIT", csoc);
            cout << "Sent data: " << res->data << endl;
            break;
        }
        else if (serverData.find(req->data) != serverData.end())
        {
            MWResponse *res = new MWResponse((char *)serverData[req->data].c_str(), csoc);
            cout << "Sent data: " << res->data << endl;
        }
        else
        {

            MWResponse *res = new MWResponse((char *)"NULL", csoc);
            cout << "Sent data: " << res->data << endl;
        }
        for (auto it = serverData.begin(); it != serverData.end(); it++)
        {
            if ((it->first.rfind("u", 0) != 0) && (stoi(it->second.substr(0, it->second.find(";"))) <= t))
            {
                cout << "Erased: " << it->first << endl;
                serverData.erase(it);
            }
        }
        t++;
    }
}