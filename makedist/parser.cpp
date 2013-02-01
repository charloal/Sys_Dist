
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include "parser.h"
#include "Node.h"

#include <algorithm>

using namespace std;

/*
 * Extract line from file
 */
string *getLine(ifstream *file)
{
  /*
	char line_char[13000];
	do {
		file->getline(line_char,13000);
//getline (in_file, *file);
#ifdef DEBUG
		cout << line_char << endl;
#endif
	} while (((strcmp(line_char,"")==0) || ('#' == line_char[0])) && (!file->eof()) );
	*/
  
	string *line = new string();
	do {
	      getline(*file, *line);
	} while ( (((*line)[0] == '#') || (line->size() == 0)) && (!file->eof()));
	return line;
}

/*
 * Extract target name from line (i.e. "target_name:")
 * Line should be well formatted
 */
string *getTarget(string *line)
{
	size_t pos = line->find(":");
	if(string::npos==pos) {
#ifdef DEBUG
		cout << "Erreur : " << *line << endl;
#endif
		return NULL;
	}
	string *target = new string(line->substr(0,pos));
	return target;
}

/*
 * Extract dependencies from line
 * If line is not well formatted, return NULL
 * Otherwise, return a list containing each dependencies (as strings)
 */
vector<string> *getDependencies(string *line)
{
#ifdef DEBUG
	cout << "GETDEPENDENCIES :" << endl;
#endif
	vector<string> *dependencies = new vector<string>;
	size_t pos = line->find(":");
	if(string::npos==pos) {
		return NULL;
	}
	replace( line->begin(), line->end(), '\t', ' ' );
	pos++;
	size_t pos_end = pos;
	while(pos_end!=string::npos) {
		pos = line->find_first_not_of(" ",pos_end);
		if(string::npos==pos) {
			break;
		}
		pos_end = line->find_first_of(" ",pos);
#ifdef DEBUG
		cout << pos << " " << pos_end << endl;
#endif
		string str = line->substr(pos,pos_end-pos);
#ifdef DEBUG
		cout << "Dep " <<  str << " (" << pos << ", " << pos_end << ")" << endl;
#endif
		dependencies->push_back(str);
	}
#ifdef DEBUG
	cout << "GETDEPENDENCIES end" << endl;
#endif
	return dependencies;	
}

/*
 * Extract commands from file and return a vector with all commands (as strings)
 */
// vector<string> *getCommands(ifstream *file)
// {
// #ifdef DEBUG
// 	cout << "GETCOMMANDS" << endl;
// #endif
// 	vector<string> *commands = new vector<string>;
// 	string *str = geLine(&file);
// 	while( (str->find(":")==string::npos)
// 			&& (strcmp(str->c_str(),"")==0)
// 			&& (!file->eof()) ) {
// 		commands->push_back(*str);
// 	}
// 	return commands;
// }

/*
 * Print each node inside nodes
 */
void printNodes(map<const char*, Node*> nodes)
{
	for(map<const char*,Node*>::iterator it=nodes.begin(); it!=nodes.end();++it) {
		Node node = *((*it).second);
		int i = node.id_global;
		cout << "\""<<  node << "\""<< endl;
		for(list<Node*>::iterator it_dep=node.listDependencies.begin();it_dep!=node.listDependencies.end();++it_dep) {
			cout << "\t cible:" << "\""<< *(*it_dep) << "\""<< endl;
		}
		for(vector<string>::iterator it_dep=node.listFilesDependencies.begin();it_dep!=node.listFilesDependencies.end();++it_dep) {
			cout << "\t files:" << "\""<< *it_dep << "\"" << endl;
		}
	}
}

/*
 * Print a dot compatible graph representation using each node inside nodes argument
 * To produce dot graph, just execute the program and get the output:
 * ./makedist > graph.dot
 * To get grapth picture, use dot:
 * dot graph.dot  -Tpng > graph.png
 */
void printGraph(vector<Node*> nodes)
{
	cout << "digraph graphName  {" << endl;
	for(vector<Node*>::iterator it=nodes.begin(); it!=nodes.end();++it) {
		Node *node = *it;
		for(list<Node*>::iterator it_dep=node->listDependencies.begin();it_dep!=node->listDependencies.end();++it_dep) {
			cout << "\t\"" << node->name << " " << node->id_machine << "\" -> \"" << *(*it_dep) << " " << (*it_dep)->id_machine <<  "\" [color=\"red\"]"<< endl;
		}
		for(list<Node*>::iterator it_dep=node->listDependant.begin();it_dep!=node->listDependant.end();++it_dep) {
			cout << "\t\"" << node->name << " " << node->id_machine << "\" -> \"" << *(*it_dep) << " " << (*it_dep)->id_machine << "\" [color=\"blue\"]"<< endl;
		}
	}
	cout << "}" << endl;
}

/*
 * Parse Makefile and produce a graph containing nodes.
 * Nodes are object describe in Node.h
 */
vector<Node*> parse(char *argv)
{
	char line[4096];
	ifstream file(argv);	// Try to open using "Makefile" filename
	if(!file.is_open()) { 	// If it doesn't work, let's try using "makefile" filename
		cerr << "no Makefile file found" << endl;
		exit(1);
	}
	vector<node_string> list_nodes_str;
	map<const char*,Node*> list_nodes;

	string *line_str = getLine(&file);
#ifdef DEBUG
	cout << "Line extracted : " << *line_str << endl;
#endif
	while(!file.eof()) {
		string *target = getTarget(line_str);
		// Get dependencies from line
		vector<string> *dependencies = getDependencies(line_str);
		// Get commands, taking next lines which doesn't contains ":"
		vector<string> *commands = new vector<string>;
		line_str = getLine(&file);
		while( (line_str->find(":")==string::npos)
				&& (strcmp(line_str->c_str(),"")!=0)
				&& (!file.eof()) ) {
#ifdef DEBUG
			cout << "command: " << *line_str << endl;
#endif
			  commands->push_back(*line_str);
			line_str = getLine(&file);
		}
		node_string node_str;
		node_str.name = *target;
		if(dependencies!=NULL) {
			node_str.listDependencies = *dependencies;
		}
		list_nodes_str.push_back(node_str);
		Node *node = new Node(*target, *commands);
		list_nodes.insert(pair<const char*,Node*>((*target).c_str(),node));
#ifdef DEBUG
		cout << "Line extracted : " << *line_str << endl;
#endif
	}
	// for each node
	for(vector<node_string>::iterator it_nodes_str=list_nodes_str.begin(); it_nodes_str!=list_nodes_str.end();++it_nodes_str) {
		node_string node_str = *it_nodes_str; // Get temp node
		// Get corresponding "real" node
		Node *node = list_nodes[node_str.name.c_str()];
#ifdef DEBUG
		cout << node_str.name.c_str() << endl;
#endif
		// for each dependency
		for(vector<string>::iterator it_dep=node_str.listDependencies.begin();it_dep!=node_str.listDependencies.end();++it_dep) {
			Node *dep = NULL;// = list_nodes[(*it_dep).c_str()];
			for(map<const char*,Node*>::iterator it_map=list_nodes.begin();it_map!=list_nodes.end();++it_map) {
#ifdef DEBUG
				cout << "Str : '"<<(*it_dep).c_str() << "'\tFirst : '" << (*it_map).first << "'\tSecond : '"<<(*it_map).second <<"' ("<< strcmp((*it_dep).c_str(),(*it_map).first)<< ")"<< endl;
#endif
				if(strcmp((*it_dep).c_str(),(*it_map).first)==0) {
					dep = (*it_map).second;
#ifdef DEBUG
					cout << "Found ! " << (*it_map).first << endl;
#endif
					if(dep!=NULL) {
						break;
					}
				}
			}
#ifdef DEBUG
			cout << "> " << (*it_dep).c_str() << " , " << *dep <<"\t" << *node << endl;
#endif
			if(/*node!=NULL &&*/ dep!=NULL) {
				node->listDependencies.push_back(dep);
				dep->listDependant.push_back(node);
			} else
			{
				node->listFilesDependencies.push_back((*it_dep).c_str());
			}
		}
	}
	//printNodes(list_nodes);

	// Convert map to vector for return
	vector<Node*> vector_nodes;
	vector_nodes.resize(list_nodes.size());
	for(map<const char*,Node*>::iterator it_map=list_nodes.begin();it_map!=list_nodes.end();++it_map) {
		Node *node = (*it_map).second;
		if (node!=NULL) {
			vector_nodes[node->id] = node;
#ifdef DEBUG
			cout << (*it_map).first << "\t" << *node << endl;
#endif
		}
	}

	return vector_nodes;
}

