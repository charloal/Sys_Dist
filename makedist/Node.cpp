
#include "Node.h"

int Node::id_global = 0;

Node::Node()
{
	id = Node::id_global++;
	id_machine	= -1;
	status = UNUSED;
}

Node::Node(string _name, vector<string> _commands)
{
	name	= _name;
	commands= _commands;
	id	= id_global++;
	id_machine	= -1;
	status = UNUSED;
}

ostream& operator<< (ostream& output, const Node& n)
{
	output<<n.name.c_str();
	return output;
}

