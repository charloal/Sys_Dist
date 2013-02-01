
#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <list>
#include <iostream>


#define UNUSED 0
#define WAITING 1
#define READY 2
#define RUNNING 3
#define DONE 4

using namespace std;

class Node
{

	friend ostream& operator<< (ostream& output, const Node& n);
	public:
		string name;
		int id;
		list<Node*> listDependencies;
		list<Node*> listDependant;
		vector<string> listFilesDependencies;
		vector<string> commands;
		
		int id_machine;
		int status;

		static int id_global;
		Node();
		Node(string name, vector<string> commands);
};

#endif

