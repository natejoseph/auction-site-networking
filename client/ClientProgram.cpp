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
#include <vector>
#include <ctime>

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
        cout << "Post item failed\n";
        break;
    case 6:
        cout << "Get items failed\n";
        break;
    case 7:
        cout << "Bid failed\n";
        break;
    case 8:
        cout << "Your bid is not the highest\n";
        break;
    case 9:
        cout << "There are no messages\n";
        break;
    case 10:
        cout << "There are no items\n";
        break;
    case 11:
        cout << "Item not found\n";
        break;
    case 12:
        cout << "Username doesn't exist\n";
        break;
    case 13:
        cout << "Key creation error\n";
        break;
    case 14:
        cout << "Value creation error\n";
        break;
    case 15:
        cout << "You are not an admin\n";
    case 16:
        cout << "Server shutdown failed\n";
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
        cout << "Price: $" << data.substr(stt, data.find(",", stt) - stt) << endl;
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
            cout << "Highest Bid: " << bidder << ", $" << data.substr(stt, data.find(",", stt) - stt) << endl;
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
        errorMessagePrint(atoi(response->data));
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
        errorMessagePrint(atoi(response->data));
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
    if (command.size() <= 11)
    {
        cout << "Invalid message\n"
             << "Format: postmessage <message>\n";
        return;
    }
    else if (command.size() == 12)
    {
        cout << "Empty message\n";
        return;
    }
    time_t now = time(0);
    char *dt = ctime(&now);
    string timestamp = string(dt);
    timestamp.pop_back();
    string msg = command.substr(12);
    msg = timestamp + ";" + msg;
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
        errorMessagePrint(atoi(response->data));
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
        errorMessagePrint(atoi(response->data));
    }
    else
    {
        cout << "Get Messages failed\n";
    }
}

void postItem(string &command, int &csoc, bool &loggedIn)
{
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }
    // Post Item send
    if (command.size() <= 9)
    {
        cout << "Invalid item post\n"
             << "Format: postitem <itemname>\n";
        return;
    }
    else if (command.size() == 10)
    {
        cout << "Empty item name\n"
             << "Item post failed\n";
        return;
    }
    string name = command.substr(9);
    cout << "Enter description:\n";
    string description;
    cout << "> ";
    getline(cin, description);
    if (description.size() == 0)
    {
        cout << "Empty description\n"
             << "Item post failed\n";
        return;
    }
    cout << "Enter price:\n";
    string price;
    cout << "> ";
    getline(cin, price);
    if (price.size() == 0)
    {
        cout << "Empty price\n"
             << "Item post failed\n";
        return;
    }
    else if (price.find_first_not_of("0123456789") != string::npos)
    {
        cout << "Invalid price\n"
             << "Item post failed\n";
        return;
    }
    time_t now = time(0);
    char *dt = ctime(&now);
    string timestamp = string(dt);
    timestamp.pop_back();
    string op = to_string(name.size() + description.size() + price.size() + timestamp.size() + 3);
    while (op.size() < 4)
    {
        op = "0" + op;
    }

    op += "PSTI";
    op += name + ";" + timestamp + ";" + price + ";" + description;
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
        errorMessagePrint(atoi(response->data));
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
        errorMessagePrint(atoi(response->data));
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
    if (line.size() == 0)
    {
        cout << "Invalid bid\n"
             << "Format: bid <itemid> <price>\n";
        return;
    }
    string item = line.substr(0, line.find(" "));
    if (item.size() == 0 || item.find_first_not_of("0123456789") != string::npos)
    {
        cout << "Invalid item id\n";
        return;
    }
    string price = line.substr(line.find(" ") + 1);
    if (price.size() == 0 || price.find_first_not_of("0123456789") != string::npos)
    {
        cout << "Invalid price\n";
        return;
    }
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
        errorMessagePrint(atoi(response->data));
    }
    else
    {
        cout << "Bid failed\n";
        cout << "Response: " << response->type << " " << response->data << "\n";
    }
}

void privateMessage(string &command, int &csoc, bool &loggedIn)
{
    if (!loggedIn)
    {
        cout << "Not logged in\n";
        return;
    }

    if (command.size() <= 15)
    {
        cout << "Invalid private message\n"
             << "Format: privatemessage <userid>\n";
        return;
    }
    else if (command.size() == 16)
    {
        cout << "Empty userid\n"
             << "Message send failed\n";
        return;
    }

    string uid = command.substr(15);
    cout << "Enter message:\n";
    string message;
    cout << "> ";
    getline(cin, message);
    if (message.size() == 0)
    {
        cout << "Empty message\n"
             << "Private message failed\n";
        return;
    }

    time_t now = time(0);
    char *dt = ctime(&now);
    string timestamp = string(dt);
    timestamp.pop_back();
    string msg = command.substr(12);
    msg = "pm::" + uid + ";" + timestamp + ";" + msg;
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
        errorMessagePrint(atoi(response->data));
    }
    else
    {
        cout << "Post error\n";
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
        errorMessagePrint(atoi(response->data));
    }
    else
    {
        cout << "Exit failed\n";
    }
}

/*void fullExit(int &csoc)
{
    char *appmsg = "0001EXITF";
    send(csoc, appmsg, 9, 0);
}*/

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
        else if (tokens[0] == "postmessage") // <sz>POST<message>
        {
            post(command, csoc, loggedIn);
        }
        else if (tokens[0] == "getmessages") // 0000GETM
        {
            getMessages(csoc, loggedIn);
        }
        else if (tokens[0] == "postitem") // <sz>PSTI<item>
        {
            postItem(command, csoc, loggedIn);
        }
        else if (tokens[0] == "getitems") // 0000GETI
        {
            getItems(csoc, loggedIn);
        }
        else if (tokens[0] == "bid") // <sz>BIDD<iid><price>
        {
            bid(command, csoc, loggedIn);
        }
        // else if (tokens[0] == "privatemessage")
        //{
        // }
        else if (tokens[0] == "exit") // 0008EXIT
        {
            exitProg(csoc);
            break;
        }
        /*else if (tokens[0] == "fullexit")
        {
            fullExit(csoc);
            break;
        }*/
        else
        {
            cout << "Invalid command. To exit, call \"exit\".\n";
        }
    }
}