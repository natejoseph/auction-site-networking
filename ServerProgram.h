#ifndef SERVERPROGRAM_H
#define SERVERPROGRAM_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <unordered_map>

using namespace std;

#endif

int recvLoop(int csoc, char *data, const int size);
char *readClientCommand(int csoc);
void serverClientInteraction(int csoc, unordered_map<string, pair<string, bool>> &users, unordered_map<string, string> &msgs);