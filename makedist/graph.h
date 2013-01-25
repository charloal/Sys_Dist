
#ifndef GRAPH_H
#define GRAPH_H

#include <list>
#include "Node.h"

using namespace std;

typedef struct _node {
	char name[256]; // Task/node name
	list<struct _node> listDependencies;
	list<struct _node> listDependant;
	int id_machine_current;

} node;

void graph_tasks_count(Node *node);

void graph_tasks_affectation_init(Node *root, int machine_id, int nb_tasks_total);

void graph_tasks_affectation(Node *node);

void graph_tasks_treatement(Node *node, int machine_id);

#endif
