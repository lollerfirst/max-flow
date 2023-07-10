#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <queue>
#include <array>
#include <vector>
#include <fstream>
#include <iomanip>
#include <unordered_set>

#define MAX_CAPACITY 100
#define VERTICES 8
#define GRAPH_FILENAME ".temp"
#define DOT_COMMAND "dot -Tsvg .temp -o img-%.4d.svg"

static int frames = 0;

enum direction_t 
{
    FORWARDS,
    BACKWARDS
};

template <typename T>
void print_graph(std::ostream& ss, const bool* edges, const T* capacity, const T* flow)
{

	ss << "digraph G{\n";
	for (int i=0; i<VERTICES; ++i)
	{
		for (int j=0; j<VERTICES; ++j)
		{
			if (edges[i*VERTICES+j])
				ss << "\t" << i << "->" << j
					<< " [label=\"" << flow[i*VERTICES+j] << "/" << capacity[i*VERTICES+j] << "\"];\n";
		}
	}
	ss << "}\n";
	ss.flush();
	
	++frames;
}


template <typename T>
void print_graph(std::ostream& ss, const bool* edges, const T* capacity, const T* flow, const std::vector<std::pair<T, direction_t>>& path)
{
	
	std::unordered_set<int> set;
	
	ss << "digraph G{\n";
	
	for (auto el : path)
	{
		set.insert(el.first);
		
		/*
		ss << "\t" << i << "->" << j
			<< " [label=\"" << flow[i*VERTICES+j] << "/" << capacity[i*VERTICES+j] << "\", color=\"blue\"];\n";
		*/
	}
	
	for (int i=0; i<VERTICES; ++i)
	{
		for (int j=0; j<VERTICES; ++j)
		{
			if (edges[i*VERTICES+j] && set.contains(i*VERTICES+j))
			{
				ss << "\t" << i << "->" << j
					<< " [label=\"" << flow[i*VERTICES+j] << "/" << capacity[i*VERTICES+j] << "\", color=\"blue\", penwidth=3];\n";
			}
			else if (edges[i*VERTICES+j])
			{
				ss << "\t" << i << "->" << j
					<< " [label=\"" << flow[i*VERTICES+j] << "/" << capacity[i*VERTICES+j] << "\"];\n";
			}
		}
	}
	
	ss << "}\n";
	
	ss.flush();
	++frames;
}

template <typename T>
std::ostream& print_matrix(std::ostream& ss, const T* mat)
{
	for (int i=0; i<VERTICES; ++i)
	{
		for (int j=0; j<VERTICES; ++j)
		{
			ss << mat[i*VERTICES+j] << "  ";
		}
		
		ss << '\n';
	}
	
	return ss;
}

template <typename T>
std::ostream& print_vector(std::ostream& ss, const std::vector<T>& path)
{
	for (const T& el : path)
	{
		if (el.second == FORWARDS)
			ss << 'f';
		else
			ss << 'b';
		
		ss << el.first << "  ";
	}
	
	ss << '\n';
	return ss;
}

bool bfs_search(std::vector<std::pair<int, direction_t>>& path, const bool* edges, const int* capacity, const int* flow, int source, int sink)
{
    bool success = false;

    std::queue<int> queue;
    std::array<int, VERTICES> pred;
    memset(pred.data(), -1, sizeof(int)*VERTICES);

    queue.push(source);

    while (!queue.empty())
    {
        int current_node = queue.front();
        queue.pop();

        if (current_node == sink)
        {
            success = true;
            break;
        }
        
        // Analyze node edges
        for (int i=0; i<VERTICES; ++i)
        {
            // Check if already visited
            if (pred[i] != -1) 
                continue;
            
            // Check if there is an edge and if it's not saturated, add it to the path
            if (edges[current_node*VERTICES+i] &&
                capacity[current_node*VERTICES+i]-flow[current_node*VERTICES+i] > 0)
            {
                pred[i] = current_node;
                queue.push(i);
            }
            // Check if there is a backwards edge from `j` to `i` and if it has flow to it, add it to the path
            else if (edges[i*VERTICES+current_node] &&
                flow[i*VERTICES+current_node] > 0)
            {
                pred[i] = current_node;
                queue.push(i);
            }
        }
               
    }

    if (success)
    {
    	int i = sink;
    	while (i != source)
	{
		int k = pred[i];
		
		if (edges[k*VERTICES+i])
		{
			path.push_back({k*VERTICES+i, FORWARDS});
		}
		else
		{
			path.push_back({i*VERTICES+k, BACKWARDS});
		}
		
		i = k;
	}
	
	//std::reverse(std::begin(path), std::end(path));
    }
    
    return success;
}

// Calculates max flow quantity for the update along the path
int delta_flow(const std::vector<std::pair<int, direction_t>>& path, const int* capacity, const int* flow)
{
    int res = INT32_MAX;
    
    for (std::pair<int, direction_t> el : path)
    {
        // Forwards edge, the flux will be increased
        if (el.second == FORWARDS)
        {
            res = std::min(res, capacity[el.first]-flow[el.first]);
        }
        // Backwards edge, the flux will be decreased
        else
        {
            res = std::min(res, flow[el.first]);
        }
    }

    return res;
}

int max_flow(const bool* edges, const int* capacity, int* flow, int source, int sink)
{
    // File to save animation graph into
    std::ofstream f(GRAPH_FILENAME, std::ios::trunc);
    print_graph(f, edges, capacity, flow);
    f.close();
    
    char cmd[256] = {0};
    sprintf(cmd, DOT_COMMAND, frames);
    system(cmd);
    
    // Augmenting path vector
    std::vector<std::pair<int, direction_t>> path;

    // Perform a breadth-first search to find an augmenting path
    while (bfs_search(path, edges, capacity, flow, source, sink))
    {
    	// Print graph with augmenting path
    	f.open(GRAPH_FILENAME, std::ios::trunc);
        print_graph(f, edges, capacity, flow, path);
        f.close();
        
        sprintf(cmd, DOT_COMMAND, frames);
        system(cmd);
    	
        // Calculate bottleneck flow quantity
        int update_quantity = delta_flow(path, capacity, flow);
        
       
        // Update flow along each node in the path by bottleneck quantity
        for (std::pair<int, direction_t> el : path)
        {
            // Increase the flux if forwards
            if (el.second == FORWARDS)
            {
                flow[el.first] += update_quantity;
            }
            // Increase the flux if backwards
            else
            {
                flow[el.first] -= update_quantity;
            }
        }
        
        // Print updated graph
        f.open(GRAPH_FILENAME, std::ios::trunc);
        print_graph(f, edges, capacity, flow);
        f.close();
        
        sprintf(cmd, DOT_COMMAND, frames);
        system(cmd);
	
	// Release current augmenting path
	path.erase(path.begin(), path.end());
    }

    int source_positive_flow = 0;
    for (int i=0; i<VERTICES; ++i)
    {
        if (edges[source*VERTICES+i])
        {
            source_positive_flow += flow[source*VERTICES+i];
        }
        else if (edges[i*VERTICES+source])
        {
            source_positive_flow -= flow[i*VERTICES+source];
        }
    }

    f.close();
    return source_positive_flow;
}

void initialize_edges(bool* edges, int* capacity)
{
	srand48(time(NULL));
	srand(time(NULL));
	
	// Initialize edges
	for (int i=0; i<VERTICES; ++i)
	{
		for (int j=0; j<VERTICES; ++j)
		{
			if (i == j)
			{
				continue;
			}
			else
			{
				if (!edges[j*VERTICES+i])
				{
					edges[i*VERTICES+j] = (drand48() <= 0.5f) ? true : false;
				}
			} 
		}
	}
	
	// Initialize capacities
	for (int i=0; i<VERTICES*VERTICES; ++i)
	{
		if (edges[i])
		{
			capacity[i] = rand() % (MAX_CAPACITY+1);
			
			if (capacity[i] == 0)
				++capacity[i];
		}
	}
	
}


int main(int argc, char** argv)
{

    bool *edges_table = (bool*) calloc(VERTICES*VERTICES, sizeof(bool));
    int *capacity_table = (int*) calloc(VERTICES*VERTICES, sizeof(int));
    int *flow_table = (int*) calloc(VERTICES*VERTICES, sizeof(int));

    // Initialize edges and capacities
    initialize_edges(edges_table, capacity_table);
    
    // Print edges and capacities
    std::cout << "Edges:\n";
    print_matrix(std::cout, edges_table);
    std::cout << "Capacities:\n";
    print_matrix(std::cout, capacity_table);
    std::cout << std::endl;
    
    // Compute max flux
    int flow = max_flow(edges_table, capacity_table, flow_table, 0, VERTICES-1);
    
    
    std::cout << "MAX FLOW: " << flow << '\n';

    free(edges_table);
    free(capacity_table);
    free(flow_table);
    return 0;
}
