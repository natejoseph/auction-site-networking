#ifndef CLIENTPROGRAM_H
#define CLIENTPROGRAM_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <unordered_map>

using namespace std;

int recvLoop(int csoc, char *data, const int size);
void clientInterface(int csoc);

#endif