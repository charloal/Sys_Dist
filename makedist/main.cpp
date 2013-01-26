
#include <mpi.h>
#include "parser.h"
#include "Machine.h"
#include <cstdlib>
#include <fstream>
#include <string>

#define TAG_TERMINATE 0
#define TAG_SIZE 1
#define TAG_WORK 2
#define TAG_RESULT 3
#define TAG_FILE 3
#define TAG_FILE_NAME 4


vector<Node*> nodes; // All nodes of the graph
list<Node*> nodes_ready;

vector<Machine*> slaves;
list<Machine*> slaves_ready;

int		machine_id	= -1;	// Our machine id
int		nb_machines	= 0;
Node *root = NULL;

string read_file(string name)
{
	ifstream in_file(name.c_str());
	string data (istreambuf_iterator<char>(in_file), (istreambuf_iterator<char>()));
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

//MPI_Send(NULL, 0, MPI_CHAR, source, TAG_RESULT, MPI_COMM_WORLD);
void send_string(string object, int source, int tag)
{
	size_t object_size = object.size() + 1;
	char* object_copy = (char*)malloc(sizeof(char)*(object_size+1));
	strncpy(object_copy, object.c_str(), object_size+1);
	MPI_Send(object_copy, object_size+1, MPI_CHAR, source, tag, MPI_COMM_WORLD);
	free(object_copy);
}


void slave()
{
	system("mkdir /tmp/alex");
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
		cout << "slave receive size " << size_command << endl;
		switch (status.MPI_TAG)
		{
		  case TAG_WORK:			
			command = (char*)malloc(sizeof(char)*(size_command+1));
			MPI_Recv(command, size_command+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);			
			cout << "slave received work:" << command <<  endl;			
			system(command);
			free(command);
			
			file = read_file(string(file_name));
// 			cout << "slave sending file_name:" << file_name << " " <<  " node:" << node->id << " to:" << machine->id << endl;
// 			send_string(file_name, machine->id, TAG_FILE_NAME);
 			cout << "slave sending file:" << file << " " <<  " file_name:" << file_name << " to:" << source << endl;
 			send_string(file, source, TAG_RESULT);
//			MPI_Send(NULL, 0, MPI_CHAR, source, TAG_RESULT, MPI_COMM_WORLD);
			break;
		  case TAG_FILE:
		    	command = (char*)malloc(sizeof(char)*(size_command+1));
			MPI_Recv(command, size_command+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
			cout << "slave received file:" << command << endl;
			write_file(file_name ,command,size_command+1);
			free(command);
			free(file_name);
			break;
		  case TAG_TERMINATE:
			return;
			break;
		  case TAG_FILE_NAME:
			file_name = (char*)malloc(sizeof(char)*(size_command+1));
			MPI_Recv(file_name, size_command+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
			cout << "slave received file_name:" << file_name << endl;
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

		
		if (nodes_ready.empty() || slaves_ready.empty())
		{
// 			cout<< nodes_ready.size() <<" sleep "<< slaves_ready.size() << endl;
			sleep(0.1);
		}
		else
		{
		  
			Node *node =  nodes_ready.front();
			nodes_ready.pop_front();
			
			Machine *machine =  slaves_ready.front();
			slaves_ready.pop_front();
			
			node->status = RUNNING;
			machine->current_node = node->id;

			//string file_name = "a";
			//MPI_Send(&file_name, 2, MPI_CHAR, machine->id, TAG_FILE_NAME, MPI_COMM_WORLD);			
			//string file = read_file("a");			
			//MPI_Send(&file, file.size(), MPI_CHAR, machine->id, TAG_FILE, MPI_COMM_WORLD);
			
			for(list<Node*>::iterator it=node->listDependencies.begin(); it!=node->listDependencies.end();++it)
			{
				string file_name = (*it)->name;
				string file = read_file(file_name);
				cout << "master sending file_name:" << file_name << " " <<  " node:" << node->id << " to:" << machine->id << endl;
				send_string(file_name, machine->id, TAG_FILE_NAME);
				cout << "master sending file:" << file << " " <<  " node:" << node->id << " to:" << machine->id << endl;
				send_string(file, machine->id, TAG_FILE);
			}
			
			string file_name = node->name;
			cout << "master sending file_name:" << file_name << " " <<  " node:" << node->id << " to:" << machine->id << endl;
			send_string(file_name, machine->id, TAG_FILE_NAME);
			
			for(vector<string>::iterator it_command=node->commands.begin();it_command!=node->commands.end();++it_command) {
			  /*
				size_t command_size = strlen((*it_command).c_str());
				char* command = (char*)malloc(sizeof(char)*(command_size+1));					
				strncpy(command, (*it_command).c_str(), command_size+1);
				cout << "master sending" << command << " " << command_size << " node:" << node->id << " to:" << machine->id << endl;
				//MPI_Send( &command_size,1, MPI_UNSIGNED_LONG, machine->id, TAG_SIZE, MPI_COMM_WORLD);
				MPI_Send(command, command_size+1, MPI_CHAR, machine->id, TAG_WORK, MPI_COMM_WORLD);
				free(command);
				*/
				 cout << "master sending command:" << (*it_command) << " " <<  " node:" << node->id << " to:" << machine->id << endl;
				 send_string((*it_command), machine->id, TAG_WORK);
			}
		}
		
		
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);		
		if (!flag)
		{
// 		      cout << "nothing received by master"<< endl;
		}
		else
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
								
				file = (char*)malloc(sizeof(char)*(size_file+1));
				MPI_Recv(file, size_file+1, MPI_CHAR, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);	
				cout << "master received file_name: "<< file_name << " file: " << file << endl;
				write_file(file_name ,file,size_file+1);
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
						if (node_1->status == READY)
						{
							nodes_ready.push_back(node_1);
						}
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
/*
	ifstream in_file("commandes");
	string data (istreambuf_iterator<char>(in_file), (istreambuf_iterator<char>()));
	const char *data2 = data.c_str();
	cout << data2 << endl;	
*/	
/*
	chdir("/tmp/alex");
	system("touch a");
*/
/*
	string data = read_file("a");
	cout << data << endl;
	
	write_file("b", data.c_str(), data.size());
*/
/*
	int i = 13;
	char* a = new char[8];
	snprintf(a, asizeof(a), "%d", i);
	cout << i << endl;
	cout << a << endl;
	cout << atoi(a) << endl;
*/	
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
