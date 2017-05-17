//Brandon Alvino
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <stack>
#include <algorithm>
#include <thread>
#include <mutex>
#include "Associative.h"
#define LINE_DELIMETER "*" //input file is strange and has three tabs at places
#define NUM_TABS 0
#define INF 1000000 
using namespace std;

//a connection to the main node, gets set up using constructor
struct connect_node
{
	int time, distance = 0;
	connect_node * next;
	string name; //for comparisons

	connect_node(string name, connect_node * next, int time, int distance)
	{
		this->name = name;
		this->next = next;
		this->time = time;
		this->distance = distance;
	}
};

//main node, used as city
struct node
{
	string name; //for comparisons
	connect_node * first; //first connection
};

//used to simulate three arrays in algorithm
struct dijk
{
	int rate = INF; //distance or time
	bool include = false;
	int path;
	node current;
};

class Network
{
private:
	Associative<node> map;
	stack<string> dist_stack, time_stack; //for outputting in reversed order
	mutex access; //for locking down the hashtable when multithreading
	int cities = 0, t_stor, d_stor;

	//gets time or distance from one node to another, assumes they are connected
	//opt is used to determine if time or distance should be evaluated, so we don't need 2 methods
	int through(dijk start, dijk destination, bool opt)
	{
		node start_city = start.current;
		node end_city = destination.current;

		//if we find it right away
		if (start_city.first->name == end_city.name && opt)
			return start_city.first->distance;

		else if (start_city.first->name == end_city.name && !opt)
			return start_city.first->time;

		//if we haven't found it then search through the nodes
		while (start_city.first->next)
		{
			if (start_city.first->next->name == end_city.name && opt)
				return start_city.first->next->distance;

			else if (start_city.first->next->name == end_city.name && !opt)
				return start_city.first->next->time;

			else
				start_city.first = start_city.first->next;
		}

		return INF; //not connected
	}

	//if we have already included a connection node
	bool included(string index_m, string compare)
	{
		node temp = map[index_m];

		while (temp.first)
		{
			if (temp.name == compare)
				return true;

			temp.first = temp.first->next;
		}
		 
		return false;
	}

	//used for outputting because we use threads and we can save space doing it this way
	void out(bool type)
	{
		if (type)
		{
			cout << "Shortest Distance: " << d_stor << " miles.\n" << "Following path: ";

			//retrive all info from stack and display it
			while (!dist_stack.empty())
			{
				cout << dist_stack.top() << " ";
				dist_stack.pop();
			}

			cout << endl;
		}

		else
		{
			cout << "Shortest time: " << t_stor << " minutes.\n" << "Following path: ";

			//retrieve all info from stack and display it
			while (!time_stack.empty())
			{
				cout << time_stack.top() << " ";
				time_stack.pop();
			}
			cout << endl;
		}
	}

	//determines if two dijk nodes are connected, very similar to through method
	bool hasConnection(dijk start, dijk destination)
	{
		node start_city = start.current;
		node end_city = destination.current;

		if (start_city.first->name == end_city.name)
			return true;

		while (start_city.first->next)
		{
			if (start_city.first->next->name == end_city.name)
				return true;

			else
				start_city.first = start_city.first->next;
		}

		return false;
	}

public:

	//adds a node to the hashtable
	bool AddNode(string data)
	{
		transform(data.begin(), data.end(), data.begin(), ::toupper); //build data to uppercase

		if (map.find(data)) //if the value isn't in the map already
			return false;

		map[data] = node();
		map[data].name = data;
		cities++;

		return true;
	}
	
	//adds connection node to a node
	bool AddConnectionNode(string data, string connection, int distance, int time)
	{
		//builds data to uppercase
		transform(data.begin(), data.end(), data.begin(), ::toupper);
		transform(connection.begin(), connection.end(), connection.begin(), ::toupper);

		//if the node isn't already in the list and the connection doesn't already exist
		if (map.find(data) && !included(data, connection)) 
		{
			connect_node * temp = new connect_node(connection, map[data].first, time, distance);
			map[data].first = temp;
			return true;
		}

		else
			return false;
	}

	//main algorithm for determining distance
	//boolean values are used so we can eliminate redundant code
	void Distance(string start, string destination, bool opt, bool isthread)
	{
		//builds strings to uppercase
		transform(start.begin(), start.end(), start.begin(), ::toupper);
		transform(destination.begin(), destination.end(), destination.begin(), ::toupper);
		t_stor = 0;
		d_stor = 0;

		//if we're not in a thread and the user entered invalid cities then leave
		if ((!map.find(start) || !map.find(destination)) && !isthread) 
		{
			cout << "Invalid start or destination entered.\nYou entered " << start << " and " << destination << endl;
			return;
		}

		//if we're in a thread and the user entered invalid cities then leave
		else if ((!map.find(start) || !map.find(destination)) && isthread)
			return;

		dijk * nodes = new dijk[cities]; //sets up our DIP array
		int counter = 0, destination_num, processing; //used for keeping track of information
		int best_rate = INF;

		access.lock(); //uses mutex so threads cannot retrive info from the hashtable at one time
					   //we can use a mutagen because we don't need to keep grabbing information, just one time
		map.first();

		do //grab information from hashtable and setup DIP array
		{
			if (map.keyvalue() == start) //logs current node to start
			{
				nodes[counter].rate = 0;
				processing = counter;
			}

			if (map.keyvalue() == destination) //logs where the destination is
				destination_num = counter;

			nodes[counter++].current = map[map.keyvalue()];
		} while (map.next());

		access.unlock(); //gives back access to hashtable for other thread

		for (int i = 0; i < counter; i++) //sets up all the paths
			nodes[i].path = processing;

		nodes[processing].include = true; //the starting node gets included

		while (!nodes[destination_num].include) //while we haven't included the destination
		{
			if (!nodes[processing].current.first) //no connections off of current node then return
				processing = nodes[processing].path;

			else
			{
				for (int i = 0; i < counter; i++) //steps through DIP array to change rate and path values
				{
					if (hasConnection(nodes[processing], nodes[i]) && nodes[i].include == false) //if the node hasn't been included and they are connected
					{
						//if we are solving for distance
						if (opt && nodes[processing].rate + through(nodes[processing], nodes[i], true) < nodes[i].rate)
						{
							nodes[i].rate = nodes[processing].rate + through(nodes[processing], nodes[i], true);
							nodes[i].path = processing;
						}

						//if we are solving for time
						else if (!opt && nodes[processing].rate + through(nodes[processing], nodes[i], false) < nodes[i].rate)
						{
							nodes[i].rate = nodes[processing].rate + through(nodes[processing], nodes[i], false);
							nodes[i].path = processing;
						}
					}
				}

				//finds the best distance that hasn't been included
				for (int i = 0; i < counter; i++)
				{
					if (nodes[i].rate < best_rate && !nodes[i].include)
					{
						processing = i;
						best_rate = nodes[i].rate;
					}
				}

				best_rate = INF; //resets distance for next round
				nodes[processing].include = true; //includes lowest distance node
			}
		}

		dijk i = nodes[processing]; //grabs the last node we were on/destination

		if (opt) //if we are solving for distance
		{
			dist_stack.push(i.current.name); //load information onto stack to output in reverse order
			d_stor = nodes[processing].rate; //stores the distance into global variable

			while (i.rate != 0) //load information
			{
				dist_stack.push(nodes[i.path].current.name);
				i = nodes[i.path]; //back up temprary variable to path
			}

			if (!isthread) //if we are not in a thread then output
				out(true);
		}

		else //we are solving for time
		{
			time_stack.push(i.current.name); //push first peice of info onto stack
			t_stor = nodes[processing].rate;

			while (i.rate != 0) //load info into stack
			{
				time_stack.push(nodes[i.path].current.name);
				i = nodes[i.path]; //back up to path
			}

			if (!isthread) //if we are not in a thread then output
				out(false);
		}

	} 

	//finds best distance and time using 2 threads
	void multithread(string start, string destination)
	{
		//sets up threads that will operate in current class, was grueling getting this to work
		//last boolean is set to true because we are going to the method via thread
		thread process1(&Network::Distance, this, start, destination, true, true);
		thread process2(&Network::Distance, this, start, destination, false, true);

		//joins our threads
		process1.join();
		process2.join();

		//outputs results serially so we don't have corrupted output
		//checks d_stor and t_stor for error reporting
		if (d_stor != 0 && t_stor != 0)
		{
			out(true);
			out(false);
		}

		else
		{
			transform(start.begin(), start.end(), start.begin(), ::toupper);
			transform(destination.begin(), destination.end(), destination.begin(), ::toupper);
			cout << "Invalid start or destination entered.\nYou entered " << start << " and " << destination << endl;
		}
	}

	//loads file given filename
	void Load(string path)
	{
		ifstream file(path);
		stringstream stream; //used for processing connections
		string info[4]; //used for processing connections
		string line, subsection; //used for processing connections
		
		if (file.good())
		{
			while (getline(file, line) && line != LINE_DELIMETER) //we have a funky delimiter for the input file from blackboard
			{
				for (int i = 0; i < NUM_TABS; i++) //gets ride of tabs by poping back
					line.pop_back();

				AddNode(line); //adds what we are left with
			}

			while (getline(file, line) && line != LINE_DELIMETER) //gets line for setting up connections
			{
				stream.clear(); //gets rid of anything left in the stream
				stream << line; //loads line into stringstream

				while (getline(stream, subsection, '\t')) //while we haven't gotten a newline character
				{
					info[0] = subsection; //funky thing to get it to work

					for (int i = 1; i < 4; i++) //gets the rest of the line
					{
						getline(stream, subsection, '\t');
						info[i] = subsection;
					}

					AddConnectionNode(info[0], info[1], stoi(info[2]), stoi(info[3]));
				}
			}

		}

	}
};


//menu
int main()
{
	Network net;
	string input, temp_a, temp_b;
	int distance, time;
	do
	{
		cout << "1. Add Node\t\t4. Shortest Distance" << endl;
		cout << "2. Add Connection\t5. Shortest Time" << endl;
		cout << "3. Load\t\t\t6. Thread Results" << endl;
		cout << "Input (-1 to quit): ";
		cin >> input;

		switch (stoi(input))
		{
		case 1: cout << "Node to add to the network: ";
			cin.ignore();
			getline(cin, temp_a);
			if (net.AddNode(temp_a))
				cout << "Succesfully added the node." << endl;
			else
				cout << "Node name was already included." << endl;

			break;

		case 2: cout << "City to add connection to: ";
			cin.ignore();
			getline(cin, temp_a);
			cout << "Connection name (must already be a node): ";
			getline(cin, temp_b);
			cout << "Distance between connections (whole number): ";
			cin >> distance;
			cout << "Time between connections (whole number in minutes): ";
			cin >> time;

			if (net.AddConnectionNode(temp_a, temp_b, distance, time))
				cout << "Sucesfully added connection node." << endl;
			else
				cout << "Error in inputs, check inputs and resubmit" << endl;

			break;

		case 3: cout << "File to load from: ";
			cin >> temp_a;
			net.Load(temp_a);
			cout << "Sucessfully loaded file to memory" << endl;

			break;

		case 4: system("cls");
			cout << "Starting city: ";
			cin.ignore();
			getline(cin, temp_a);
			cout << "Destination city: ";
			getline(cin, temp_b);
			cout << endl;
			net.Distance(temp_a, temp_b, true, false);

			break;

		case 5: system("cls");
			cout << "Starting city: ";
			cin.ignore();
			getline(cin, temp_a);
			cout << "Destination city: ";
			getline(cin, temp_b);
			cout << endl;
			net.Distance(temp_a, temp_b, false, false);

			break;

		case 6: system("cls");
			cout << "Starting city: ";
			cin.ignore();
			getline(cin, temp_a);
			cout << "Destination city: ";
			getline(cin, temp_b);
			cout << endl;
			net.multithread(temp_a, temp_b);

			break;

		default: cout << "Invalid menu option submitted." << endl;
		}

		system("pause");
		system("cls");
	} while (input != "-1");
	

	//I MESSED UP THE FILE INPUT SO WAS GETTING INVALID RESULTS
	//LOOKS WHAT I HAD TO DO TO FIND OUT THAT IT WAS AN ERROR IN THE FILE LOAD

	//n.AddConnection("Minot", "Rugby", 65, 60);
	//n.AddConnection("Rugby", "Devils Lake", 58, 54);
	//n.AddConnection("Devils Lake", "Grand Forks", 90, 83);
	//n.AddConnection("Grand Forks", "Fargo", 80, 60);
	//n.AddConnection("Minot", "Carrington", 127, 60);
	//n.AddConnection("Carrington", "Jamestown", 43, 60);
	//n.AddConnection("Rugby", "Carrington", 98, 60);
	//n.AddConnection("Jamestown", "Valley City", 36, 60);
	//n.AddConnection("Valley City", "Fargo", 62, 60);
	//n.AddConnection("Minot", "Stanley", 55, 60);
	//n.AddConnection("Stanley", "Williston", 73, 60);
	//n.AddConnection("Stanley", "New Town", 32, 60);
	//n.AddConnection("New Town", "Williston", 70, 60);
	//n.AddConnection("Dickinson", "Williston", 131, 60);

	//n.AddConnection("Dickinson", "New Town", 96, 60);
	//n.AddConnection("Jamestown", "Grand Forks", 151, 54);
	//n.AddConnection("Minot", "Beulah", 97, 83);
	//n.AddConnection("Minot", "Bismarck", 110, 60);
	//n.AddConnection("Beulah", "Dickinson", 78, 60);
	//n.AddConnection("Bismarck", "Dickinson", 98, 60);
	//n.AddConnection("Bismarck", "Jamestown", 102, 60);
	//n.AddConnection("Minot", "New Town", 74, 60);
	//n.AddConnection("Rugby", "Minot", 65, 60);
	//n.AddConnection("Devils Lake", "Rugby", 58, 60);
	//n.AddConnection("Grand Forks", "Devils Lake", 90, 60);
	//n.AddConnection("Fargo", "Grand Forks", 80, 60);
	//n.AddConnection("Carrington", "Minot", 127, 60);
	//n.AddConnection("Jamestown", "Carrington", 43, 60);

	//n.AddConnection("Carrington", "Rugby", 98, 60);
	//n.AddConnection("Valley City", "Jamestown", 36, 54);
	//n.AddConnection("Fargo", "Valley City", 62, 83);
	//n.AddConnection("Stanley", "Minot", 55, 60);
	//n.AddConnection("Williston", "Stanley", 73, 60);
	//n.AddConnection("New Town", "Stanley", 32, 60);
	//n.AddConnection("Williston", "New Town", 70, 60);
	//n.AddConnection("Williston", "Dickinson", 131, 60);
	//n.AddConnection("New Town", "Dickinson", 96, 60);
	//n.AddConnection("Grand Forks", "Jamestown", 151, 60);
	//n.AddConnection("Beulah", "Minot", 97, 60);
	//n.AddConnection("Bismarck", "Minot", 110, 60);
	//n.AddConnection("Dickinson", "Beulah", 78, 60);
	//n.AddConnection("Dickinson", "Bismarck", 98, 60);
	//n.AddConnection("Jamestown", "Bismarck", 102, 60);
	//n.AddConnection("New Town", "Minot", 74, 60);

	//n.Distance("Minot", "Stanley");

	system("pause");
}