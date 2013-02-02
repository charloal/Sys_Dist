
#include <mpi.h>
#include "parser.h"
#include "Machine.h"
#include <cstdlib>
#include <fstream>
#include <string>
#include <algorithm>

#define TAG_TERMINATE 0
#define TAG_SIZE 1
#define TAG_WORK 2
#define TAG_RESULT 3
#define TAG_FILE 3
#define TAG_FILE_NAME 4

//#define COUT
//#define ENVOI_INTELLIGENT


vector<Node*> nodes; // All nodes of the graph
list<Node*> nodes_ready;

vector<Machine*> slaves;
list<Machine*> slaves_ready;

int machine_id = -1; // Our machine id
int nb_machines = 0;
Node *root = NULL;

string read_file(string name)
{
	ifstream in_file;
	in_file.open(name.c_str());
	string data ="", temp="";
	while (!in_file.eof())
	{	 
	   getline (in_file, temp);
	   data.append(temp);
	   if (!in_file.eof())
	   data.append("\n");
	}
	in_file.close();
	return data;
}

void write_file (string name, char* text, int size)
{
	ofstream stream;
	stream.open(name.c_str());
	stream.write(text, size);
	stream.close();
}

void execute(Node *node)
{
	for(vector<string>::iterator it_command=node->commands.begin();it_command!=node->commands.end();++it_command) {
		size_t command_size = strlen((*it_command).c_str());
                char* command = (char*)malloc(sizeof(char)*(command_size+1));		
                strncpy(command, (*it_command).c_str(), command_size+1);
		cout<<command<<endl;
		system(command);
		free(command);
	}

}

void activate(Node *node)
{	
	if (node->listDependencies.size() == 0 )
	{
		if (node->status != READY)
		{
			node->status = READY;
			nodes_ready.push_back(node);
		}
	} else
	{
		node->status = WAITING;
		for(list<Node*>::iterator it=node->listDependencies.begin(); it!=node->listDependencies.end();++it) {
			Node *node_f = *it;
			activate(node_f);
		}
	}
	
}

Machine* findBestMachine(Node *node)
{
	double max_per = -1;
	double max_per_local = 0;
	Machine *best_machine;
  
	for(list<Machine*>::iterator machine=slaves_ready.begin(); machine!=slaves_ready.end();++machine) 
	{
		
		for(list<Node*>::iterator it=node->listDependencies.begin(); it!=node->listDependencies.end();++it)
		{	
			string file_name = (*it)->name;
			if (find ((*machine)->files.begin(), (*machine)->files.end(), file_name) != (*machine)->files.end())
			{
				max_per_local++;
			}
		}
		
		for(vector<string>::iterator it=node->listFilesDependencies.begin(); it!=node->listFilesDependencies.end();++it)
		{
			string file_name = (*it);
			if (std::find ((*machine)->files.begin(), (*machine)->files.end(), file_name) != (*machine)->files.end())
			{
				max_per_local++;
			}
		}
		if (max_per_local > max_per)
		{
			max_per = max_per_local;
			best_machine = *machine;
		}
#ifdef COUT
		cout << (*machine)->id << " " << max_per_local << endl;
#endif
		max_per_local = 0;
		
	}
	return best_machine;
}

void send_string(string object, int source, int tag)
{
	size_t object_size = object.size() + 1;
	char* object_copy = (char*)malloc(sizeof(char)*(object_size));
	strncpy(object_copy, object.c_str(), object_size);
	MPI_Send(object_copy, object_size, MPI_CHAR, source, tag, MPI_COMM_WORLD);
	free(object_copy);
}


void slave()
{
	//system("mkdir /tmp/alex");
	chdir("/tmp/alex");
  
	char *command, *file_name;
	string file;
	int source = 0;
	MPI_Status status;
	
	int size_command;
	
	while(true)
	{
		MPI_Probe(source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_CHAR, &size_command);
		switch (status.MPI_TAG)
		{
		  case TAG_WORK:
			command = (char*)malloc(sizeof(char)*(size_command+1));
			MPI_Recv(command, size_command+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			system(command);
			free(command);
			file = read_file(string(file_name));
 			send_string(file, source, TAG_RESULT);
			break;
		  case TAG_FILE:
		    	command = (char*)malloc(sizeof(char)*(size_command+1));
			MPI_Recv(command, size_command+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			write_file(file_name ,command,size_command-1);
			free(command);
			free(file_name);
			break;
		  case TAG_TERMINATE:
			return;
			break;
		  case TAG_FILE_NAME:
			file_name = (char*)malloc(sizeof(char)*(size_command+1));
			MPI_Recv(file_name, size_command+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			break;
		}
	}
}

void master()
{

	int flag, source, size_file;
	MPI_Status status;
	char *file, *file_name;
	
  
	while (root->status != READY)
	{
	  
		while (!(nodes_ready.empty() || slaves_ready.empty()))
		{
		  
			Node *node =  nodes_ready.front();
			nodes_ready.pop_front();
			
#ifdef ENVOI_INTELLIGENT			
			Machine *machine = findBestMachine(node);
			slaves_ready.remove(machine);
#else			
			Machine *machine =  slaves_ready.front();
			slaves_ready.pop_front();
#endif			
			node->status = RUNNING;
			machine->current_node = node->id;

			for(list<Node*>::iterator it=node->listDependencies.begin(); it!=node->listDependencies.end();++it)
			{			
				string file_name = (*it)->name;
				if (find (machine->files.begin(), machine->files.end(), file_name) == machine->files.end())
				{
					string file = read_file(file_name);
#ifdef COUT
					cout << "master sending file:" << file_name << " " <<  " node:" << node->id << " to:" << machine->id << endl;
#endif
			
					send_string(file_name, machine->id, TAG_FILE_NAME);
					send_string(file, machine->id, TAG_FILE);
					
					machine->files.push_back(file_name);
				}
			}
			
			for(vector<string>::iterator it=node->listFilesDependencies.begin(); it!=node->listFilesDependencies.end();++it)
			{
				string file_name = (*it);
				if (find (machine->files.begin(), machine->files.end(), file_name) == machine->files.end())
				{
					string file = read_file(file_name);
#ifdef COUT
					cout << "master sending file:" << file_name << " " <<  " node:" << node->id << " to:" << machine->id << endl;
#endif
					send_string(file_name, machine->id, TAG_FILE_NAME);
					send_string(file, machine->id, TAG_FILE);
					
					machine->files.push_back(file_name);
				}
			}
			
			string file_name = node->name;
#ifdef COUT
			cout << "master sending file_name:" << file_name << " " <<  " node:" << node->id << " to:" << machine->id << endl;
#endif
			send_string(file_name, machine->id, TAG_FILE_NAME);
			
			for(vector<string>::iterator it_command=node->commands.begin();it_command!=node->commands.end();++it_command)
			{  
#ifdef COUT
				 cout << "master sending command:" << (*it_command) << " " <<  " node:" << node->id << " to:" << machine->id << endl;
#endif
				 send_string((*it_command), machine->id, TAG_WORK);
			}
		}
		
		
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
		if (flag)
		{
			switch (status.MPI_TAG)
			{
			  case TAG_RESULT:
				MPI_Get_count(&status, MPI_CHAR, &size_file);
			    
				source = status.MPI_SOURCE;
				slaves_ready.push_back(slaves[source]);

				Node *node = nodes[slaves[source]->current_node];
				node->status = DONE;

				file_name = (char*)malloc(sizeof(char)*( node->name.size() + 1));
				strncpy(file_name,  node->name.c_str(), node->name.size() + 1);
	
				file = (char*)malloc(sizeof(char)*(size_file));
				MPI_Recv(file, size_file, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
#ifdef COUT	
				cout << "master received file: "<< file_name << " from: " << source << " size: " << size_file << endl;
#endif
				write_file(file_name ,file,size_file-1);
				
				slaves[source]->files.push_back(file_name);
				
				free(file);
				free(file_name);

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
						if ((node_1->status == READY) && (node_1 != root))
						{
							nodes_ready.push_back(node_1);
						}
					}
				}
			}
		}
	}
	
	cout << "master execute: ";
	execute(root);
	
	for (int i=1; i<nb_machines; ++i)
	{
		MPI_Send(NULL, 0, MPI_CHAR, i, TAG_TERMINATE, MPI_COMM_WORLD);
#ifdef COUT
		cout << i << ": ";
		for(vector<string>::iterator it=slaves[i]->files.begin(); it!=slaves[i]->files.end();++it)
		{
			cout << (*it) << " " ;
		} 
		cout << endl;
#endif
	}
}



void init(int argc, char **argv)
{	
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&machine_id);
	MPI_Comm_size(MPI_COMM_WORLD,&nb_machines);
	
	if (machine_id == 0)
	{
		chdir("/tmp/alex") ;
		nodes = parse(argv[1]);
		
		if(argc>2) {
			for(vector<Node*>::iterator it=nodes.begin();it!=nodes.end();++it) {
				if(!strcmp((*it)->name.c_str(),argv[2])) {
					root = *it;
				}
			}
		} else { 
			root = nodes[0];
		}
		if(NULL==root) {
			cerr << "No Makefile target ";
			if(argc>2) {
				cerr << "("<< argv[2] << ") ";
			}
			cerr << "found" << endl;
			exit(1);
		}
		
		if((root->commands.size() == 0) && (root->listDependencies.size() == 1))
		{
			root = root->listDependencies.front();
		}
		activate(root);
#ifdef COUT
		printGraph(nodes);
#endif
	
		Machine *machine = new Machine(0);
		slaves.push_back(machine);
	

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
