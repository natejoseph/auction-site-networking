#ifndef MIDDLEWAREPROGRAM_H
#define MIDDLEWAREPROGRAM_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <unordered_map>

using namespace std;

int recvLoop(int csoc, char *data, const int size);
void middlewareClientInteraction(int csoc, int ssoc);

#endif