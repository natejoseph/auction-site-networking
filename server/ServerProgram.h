#ifndef SERVERPROGRAM_H
#define SERVERPROGRAM_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <unordered_map>

using namespace std;

int recvLoop(int csoc, char *data, const int size);
void serverMiddlewareInteraction(int csoc);

#endif