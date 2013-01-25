
#include "graph.h"

/*
 * Global variables
 */
extern int	nb_machines;
extern vector<Node*>	nodes_ready;	// Nodes current machine can work on
extern vector<Node*>	nodes_blocked; //  Nodes current machine is waiting for

int	current_machine_id	= -1;
int	nb_tasks_per_machine	= -1;
int	nb_tasks_on_current_machine	= 0;
int nb_tasks_total	= 0; // Total number of tasks
int nb_tasks	= 0;	// Only tasks dependant of root

void graph_tasks_affectation_init(Node *root, int machine_id, int nb_tasks_total)
{
	current_machine_id	= 0;
	// First call: only to count number of nodes we will walk
	/*nb_tasks_per_machine	= nb_tasks_total/nb_machines;
	if(nb_tasks_total%nb_machines!=0) {
		nb_tasks_per_machine++;
	}*/
//	cout << "nb_tasks_per_machine " << nb_tasks_per_machine << endl;
	//graph_tasks_treatement(root);
	graph_tasks_count(root);

	// Second call: real graph coloration
	nb_tasks_per_machine	= nb_tasks/nb_machines;
	if(nb_tasks%nb_machines!=0) {
		nb_tasks_per_machine++;
	}
//	cout << "nb_tasks_per_machine " << nb_tasks_per_machine << endl;
	graph_tasks_treatement(root, machine_id);
}

void graph_tasks_count(Node *node)
{
	if(node->id_machine==-1) {
		node->id_machine=-2;
		nb_tasks++;
		for(list<Node*>::iterator it=node->listDependencies.begin();it!=node->listDependencies.end();++it) {
			graph_tasks_count(*it);
		}
	}
}

void graph_tasks_treatement(Node *node, int machine_id)
{
	if(node->id_machine<0) { // No machine affected to node yet
		// First, we treat all node's childs
		for(list<Node*>::iterator it=node->listDependencies.begin();it!=node->listDependencies.end();++it) {
			graph_tasks_treatement(*it, machine_id);
		}
		// Next, we apply treatement on current node
		graph_tasks_affectation(node);
		if(current_machine_id==machine_id) {
			if(node->listDependencies.empty()) {
				nodes_ready.push_back(node);
			} else {
				nodes_blocked.push_back(node);
			}
		}
		// Finally, couting number of tasks affected to current node
		nb_tasks_on_current_machine++;
		if(nb_tasks_on_current_machine==nb_tasks_per_machine) {
			current_machine_id++;
			nb_tasks_on_current_machine=0;
		}
	}
}

inline void graph_tasks_affectation(Node *node)
{
	node->id_machine	= current_machine_id;
}

