
#ifndef MACHINE_H
#define MACHINE_H

#include <string>
#include <vector>
#include <list>
#include <iostream>


using namespace std;

class Machine
{

	public:
		int id;
		//list<> files;
		int current_node;
		int status;

		vector<string> files;
		
		Machine();
		Machine(int _id);
};

#endif