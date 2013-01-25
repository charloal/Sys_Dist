
#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "Node.h"
//#include "graph.h"
//#define DEBUG 1

typedef struct _node_string {
	string name;
	vector<string> listDependencies;
} node_string;

/*
* parse file using "Makefile" or "makefile" file name inside current directory
*/
vector<Node*> parse(char *argv);

string *getLine(ifstream *file);

string *getTarget(string *line);

vector<string> *getDependencies(string *line);

vector<string> *getCommands(ifstream *file);

void printGraph(vector<Node*> nodes);

#endif

