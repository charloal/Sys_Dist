
#include <mpi.h>
#include "parser.h"
#include "Machine.h"
#include <cstdlib>

#define TAG_TERMINATE 0
#define TAG_WORK_SIZE 1
#define TAG_WORK 2
#define TAG_RESULT 3



vector<Node*> nodes; // All nodes of the graph
list<Node*> nodes_ready;

vector<Machine*> slaves;
list<Machine*> slaves_ready;

int		machine_id	= -1;	// Our machine id
int		nb_machines	= 0;
Node *root = NULL;

void execute(Node *node)
{
	for(vector<string>::iterator it_command=node->commands.begin();it_command!=node->commands.end();++it_command) {
		size_t command_size = strlen((*it_command).c_str());
                char* command = (char*)malloc(sizeof(char)*(command_size+1));		
                strncpy(command, (*it_command).c_str(), command_size+1);
		cout<<command<<endl;
		system(command);
	}

}

void activate(Node *node)
{	
	if (node->listDependencies.size() == 0 )
	{
		 node->status = READY;
		 nodes_ready.push_back(node);
	} else
	{
		node->status = WAITING;
		for(list<Node*>::iterator it=node->listDependencies.begin(); it!=node->listDependencies.end();++it) {
			Node *node_f = *it;
			activate(node_f);			
		}
	}
	
}


void slave()
{
	char *command;
	int source = 0;
	MPI_Status status;
	
	command = new char[255];
	
	while(true)
	{
		MPI_Recv(command, 255, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == TAG_WORK)
		{
			cout << "slave received" << command << endl;
			system(command);
			cout << "slave sending" << endl;
			MPI_Send(NULL, 0, MPI_CHAR, source, TAG_RESULT, MPI_COMM_WORLD);
		}
		else if (status.MPI_TAG == TAG_TERMINATE)
		{
			return;
		}
	}
}

void master()
{  
	int flag, source;
	MPI_Status status;
	

  
	while (root->status != READY)
	{

		
		if (nodes_ready.empty() || slaves_ready.empty())
		{
			cout<< nodes_ready.size() <<" sleep "<< slaves_ready.size() << endl;
			sleep(1);
		}
		else
		{
			cout << "MASTER SENDING ????" << endl;
		  
			Node *node =  nodes_ready.front();
			nodes_ready.pop_front();
			
			cout << node->id << endl;
			
			Machine *machine =  slaves_ready.front();
			slaves_ready.pop_front();
			
			node->status = RUNNING;
			machine->current_node = node->id;
			
			cout << machine->id << endl;			
			
			for(vector<string>::iterator it_command=node->commands.begin();it_command!=node->commands.end();++it_command) {
				size_t command_size = strlen((*it_command).c_str());
				char* command = (char*)malloc(sizeof(char)*(command_size+1));		
				strncpy(command, (*it_command).c_str(), command_size+1);
				cout << "master sending" << command << " " << command_size << " node:" << node->id << " to:" << machine->id << endl;
				MPI_Send(command, command_size, MPI_CHAR, machine->id, TAG_WORK, MPI_COMM_WORLD);
			}
		}
		
		
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);		
		if (!flag)
		      cout << "nothing received by master"<< endl;
		else
		{
			source = status.MPI_SOURCE;
			cout << "master received " << source << " " << slaves[source]->current_node << endl;
			slaves_ready.push_back(slaves[source]);
			
			MPI_Recv(NULL, 0, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			Node *node = nodes[slaves[source]->current_node];
			node->status = DONE;
			
			for(list<Node*>::iterator it=node->listDependant.begin(); it!=node->listDependant.end();++it) {
				Node *node_1 = *it;
				if (node_1->status == WAITING)
				{				
					for(list<Node*>::iterator it_2=node_1->listDependencies.begin(); it_2!=node_1->listDependencies.end();++it_2) 
					{
						Node *node_2 = *it_2;
						if (node_2->status == DONE)
						{
							node_1->status = READY;
						} else
						{
							node_1->status = WAITING;
							break;
						}
					}
					if (node_1->status == READY)
					{
						nodes_ready.push_back(node_1);
					}
				}
			}		
		}
	}
	
	execute(root);
	
	for (int i=1; i<nb_machines; ++i)
	{
		MPI_Send(NULL, 0, MPI_CHAR, i, TAG_TERMINATE, MPI_COMM_WORLD);
	}
}



void init(int argc, char **argv)
{	
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&machine_id);
	MPI_Comm_size(MPI_COMM_WORLD,&nb_machines);
	
	if (machine_id == 0)
	{
		nodes = parse(argv[1]);
		
		if(argc>2) {
			for(vector<Node*>::iterator it=nodes.begin();it!=nodes.end();++it) {
				if(!strcmp((*it)->name.c_str(),argv[2])) {
					root	= *it;
				}
			}
		} else { 
			root	= nodes[0];
		}
		if(NULL==root) {
			cerr << "No Makefile target ";
			if(argc>2) {
				cerr << "("<< argv[2] << ") ";
			}
			cerr << "found" << endl;
			exit(1);
		}
		
		activate(root);
		printGraph(nodes);


		Machine *machine = new Machine(0);
		slaves.push_back(machine);
	
		//cout << nb_machines << " " << machine_id<< endl;
		for(int i=1; i<nb_machines; ++i)
		{
			Machine *machine = new Machine(i);
			slaves.push_back(machine);
			slaves_ready.push_back(machine);
		}
	}
	
}

void finish()
{
	MPI_Finalize();
}



/*
 * Program main
 */
int main(int argc, char **argv)
{

    	
	if(argc<2) {
 		cerr << "No Makefile " << endl;
		exit(1);		
	}
  	
	init(argc, argv);
	
	if(machine_id == 0)
	{
	  master();
	}
	else
	{
	  slave();
	}
	
	finish();	
		
	return 0;
}
