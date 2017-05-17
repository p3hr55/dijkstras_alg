#pragma once
#pragma once
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <string>
#include <iostream>
using namespace std;
template <class T> class Associative
{
private:
	struct Node
	{
		string key;
		T data;
		Node * next = NULL;
	};

	struct DataEntry //used for outputting
	{
		int col;
		int ts;
	};

public:
	Associative() //default constructor 
	{
		d = new Node[11];
		size = 11;
	}

	Associative(int n) //selected size constructor, creates size equal to next prime
	{
		size = create(n);
		d = new Node[size];
	}

	int gsize() //returns the size of the table
	{
		return size;
	}

	bool first() //sets our current index to -1 and calls the next function, 
	{            //this works because the next instruction imediately increments
		curr_index = -1;
		return next();
	}

	string keyvalue() //returns the current nodes key
	{
		return p.key;
	}

	T datavalue() //returns the current nodes data
	{
		return p.data;
	}

	bool next() //moves current node to next position
	{
		int c = curr_index + 1;

		if (p.next) //if we have a next node than just make current equal to it
		{
			p = *(p.next);
			return true;
		}

		if (c == size) //needed for advanced error checking
			return false;

		while (d[c].key == "") //if we don't have a next node then we need to search for one
		{
			if (++c == size)
				return false;
		}

		p = d[c]; //sets current node equal to next node
		curr_index = c; //adjusts the index of the current node
		return true;
	}

	bool find(string s) //finds a node if its in the table
	{
		int n = hash(s);

		if (d[n].key == s) //if the item to find is the first item
			return true;

		else if (d[n].next != NULL) //if its not then we must search for it
		{
			Node * temp = &d[n];

			while (temp->next) //traverse the linked list
			{
				if (temp->next->key == s) //if we find it then return it
					return true;

				temp = temp->next; //advance temp
			}
		}

		return false;
	}

	bool remove(string s) //removes a node in the array
	{
		if (find(s)) //if the node exists then we can delete it
		{
			node_count--; //we are getting rid of a node
			Node * temp;
			int n = hash(s);

			if (d[n].key == s) //if the node is the first item
			{
				if (d[n].next) //if the node has a next value then hook it up
				{
					temp = d[n].next;
					d[n].key = temp->key;
					d[n].data = temp->data;
					d[n].next = temp->next;
					temp = NULL;
					delete temp;
				}
				else //set the node to null
				{
					Node * s = &d[n];
					d[n].key = "";
				}


			}

			else if (d[n].next) //if the node isn't first then we must find its location
			{
				Node * temp2 = &d[n];

				while (temp2->next) //traverse the linked list
				{
					if (temp2->key == s) //if we find it determine how to delete it
					{
						if (temp2->next)
						{
							temp = temp2->next;
							temp2->key = temp->key;
							temp2->data = temp->data;
							temp2->next = temp->next;
							temp = NULL;
							delete temp;
						}

						else
						{
							temp2->key = "";
							delete[] temp2;
						}
					}
				}
			}
			return true;
		}

		return false;
	}

	T & operator[](string i) //overloads bracket operator
	{
		if (check()) //if the table is halfway full then rehash
			rehash();

		int n = hash(i);

		if (d[n].key == "" || find(i)) //if we are inserting a first node or retrieving/changing
		{
			if (d[n].key == i) //if we are replacing a node or printing it out
				return d[n].data;

			Node * temp = &d[n];
			while (temp->next != NULL) //search to see if we can find the node
			{
				temp = temp->next;

				if (temp->key == i)
					return temp->data;
			}

			if (temp->key != i) //create a new node
			{
				node_count++;
				d[n].key = i;
				return d[n].data;
			}
		}

		else if (d[n].key != "" || d[n].key != i) //if we need to link something up
		{
			collisions++; //we have a collision
			node_count++;
			Node * temp = &d[n];

			if (temp->next == NULL) //if the nexxt value is null then hook it up
			{
				temp->next = new Node();
				temp->next->key = i;
				return temp->next->data;
			}

			else //we need to search for where to hook it up
			{
				while (temp->next)
				{
					if (temp->next->key == i) //if we find the value then just return it
						return d[n].data;
					temp = temp->next; //advance temp
				}

				//setup node
				temp->next = new Node();
				temp->next->key = i;
				return temp->next->data;
			}
		}
	}

	int number_of_nodes() //used for debuging purposes
	{
		return node_count;
	}

	~Associative() //reports collisions and reclaims memory
	{
		cout << "\nCollisions status for runtime of table:" << endl;
		for (int i = 0; i < e_count; i++)
			cout << "Experienced " << r[i].col << " collisions with a table size of " << r[i].ts << endl;

		if (size != r[e_count].ts)
			cout << "Experienced " << collisions << " collisions with a table size of " << size << endl;

		delete[] d;
	}

private:
	int size, t_size, curr_index, collisions = 0, node_count = 0, e_count = 0; //useful information
	DataEntry r[500]; //used for outputting assumes we will only resize a max of 500 times
	Node * d; //the table
	Node p; //the current node

	int create(int n) //generates primes
	{
		int i = n;
		bool p;

		if (i % 2 == 0) //we will never be able to generate a prime from an even
			i++;

		while (true)
		{
			p = true;

			for (int x = 2; x < sqrt(i); x++) //we only need to check up until the sqrt
			{
				if (i % x == 0) //if its divisible then we havent found a prime
				{
					p = false;
					break;
				}
			}

			if (p) //we have a prime number that suffices
				return i;

			i += 2; //no point incrementing because evens can never be prime
		}
	}

	int hash(string s) //computes a hash value
	{
		int n = 0;
		for (int i = 0; i < s.length(); i++)
		{
			n = (n * 101) + s[i];
		}

		return abs(n % size); //mods it by the table size so there is no index out of bounds exceptions
	}

	bool check() //procedure to check if we should rehash
	{
		if (node_count * 2 > size)
			return true;

		return false;
	}

	void rehash() //perfroms when the hashtable is at half capacity
	{
		int t_node = node_count;
		r[e_count].col = collisions;
		r[e_count++].ts = size;
		t_size = size; //stores the size in a temporary value
		collisions = 0; //resets our nodes and collisions because they will increase when adding back
		Node * temp = new Node[size]; //make temporary storage space
		int count = 0;
		int c2 = 0;

		first(); //put current node to first node location

		do
		{
			temp[count].key = keyvalue(); //loads the key and data into the storage
			temp[count++].data = datavalue();
			remove(keyvalue());
		} while (next());

		size = create(size * 2); //resize variable
		delete[] d;
		d = new Node[size]; //create new table

		for (int i = 0; i < t_size; i++) //reload each item back into the table
		{
			if (temp[i].key != "") //makes sure we don't add invalid stuff to the table
				(*this)[temp[i].key] = temp[i].data; //calls the add routine of current object
		}

		node_count = t_node;
		delete[] temp;
	}
};