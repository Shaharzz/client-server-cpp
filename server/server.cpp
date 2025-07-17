
#include <iostream>     // Input and output stream, provides functionality for basic console input and output operations.
#include <fstream>      // File stream, used for file input and output operations.
#include <vector>       // Vector container, provides dynamic array functionality.
#include <string>       // String class, used for manipulating sequences of characters.
#include <sstream>      // String stream, used for parsing and formatting strings.
#include <cstring>      // C-style string manipulation functions, for working with null-terminated character arrays.
#include <algorithm>    // Algorithms for operating on containers, such as sorting and searching.
#include <thread>       // Threading support, for creating and managing concurrent execution.
#include <mutex>        // Mutual exclusion primitives, for protecting shared resources in multithreaded programs.
#include <unordered_map> // Unordered associative container, provides a hash-based map data structure.
#include <queue>        // Queue container, provides FIFO (First In, First Out) data structure.
#include <sys/socket.h> // Socket interface, for creating and managing network sockets.
#include <netinet/in.h> // Internet address family, defines structures and constants for IP addresses.
#include <arpa/inet.h>  // Functions for manipulating numeric IP addresses, such as conversion between binary and text representations.
#include <unistd.h>     // Standard symbolic constants and types, includes various POSIX standard functions.


using namespace std; // Allows usage of standard library components without prefixing them with std::

// Structure to represent an edge
struct Edge {
    int u, v;
    Edge(int u, int v) : u(u), v(v) {}
};

// Structure to represent the graph
class Graph {
public:
    int V; // Number of vertices
    vector<vector<int>> adj; // Adjacency list

    Graph(int V) : V(V), adj(V) {} // Constructor initializing the number of vertices and adjacency list

    // Function to add an edge to the graph
    void addEdge(int u, int v) {
        adj[u].push_back(v); // Add v to the adjacency list of u
        adj[v].push_back(u); // Add u to the adjacency list of v (assuming an undirected graph)
    }
};

// Function to read and parse the CSV file representing the graph
Graph parseGraphFile(const string& filename) {
    ifstream file(filename); // Open the file
    string line; // String to store each line of the file
    int max_vertex = -1; // Variable to store the maximum vertex number

    if (!file.is_open()) { // Check if the file opening was successful
        cerr << "Error: Unable to open file " << filename << endl; // Print error message if file opening failed
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Find the maximum vertex number to determine the size of the graph
    while (getline(file, line)) { // Read each line of the file
        stringstream ss(line); // Create a stringstream to parse the line
        int u, v; // Variables to store the vertex numbers
        ss >> u >> v; // Extract the vertex numbers from the line
        max_vertex = max({max_vertex, u, v}); // Update max_vertex with the maximum of itself and the current vertex numbers
    }

    Graph graph(max_vertex + 1); // Create a graph with size max_vertex + 1

    // Reset file pointer to the beginning of the file
    file.clear(); // Clear any error flags on the file
    file.seekg(0); // Set the file pointer to the beginning of the file

    // Parse each line to create the graph
    while (getline(file, line)) { // Read each line of the file
        stringstream ss(line); // Create a stringstream to parse the line
        int u, v; // Variables to store the vertex numbers
        ss >> u >> v; // Extract the vertex numbers from the line
        graph.addEdge(u, v); // Add an edge between vertices u and v to the graph
    }

    file.close(); // Close the file
    return graph; // Return the constructed graph
}

// Function to compute shortest path using BFS algorithm
vector<int> shortestPath(const Graph& graph, int src, int dest) {
    // Initialize parent array to store the parent of each vertex in the shortest path
    vector<int> parent(graph.V, -1); // Create a vector of size graph.V (number of vertices) initialized with -1
    // Initialize visited array to mark visited vertices
    vector<bool> visited(graph.V, false); // Create a vector of size graph.V initialized with false
    // Initialize a queue to perform BFS traversal
    queue<int> q; // Create an empty queue of integers

    // Start BFS traversal from the source vertex
    q.push(src); // Push the source vertex onto the queue
    visited[src] = true; // Mark the source vertex as visited

    while (!q.empty()) { // Continue BFS until the queue is empty
        int u = q.front(); // Get the front element of the queue (current vertex)
        q.pop(); // Remove the front element from the queue

        // If destination vertex is found, break out of the loop
        if (u == dest) break;

        // Traverse all adjacent vertices of u
        for (int v : graph.adj[u]) { // Iterate over each adjacent vertex v of u
            // If v is not visited, mark it as visited, set its parent to u, and push it to the queue
            if (!visited[v]) { // Check if v is not visited
                visited[v] = true; // Mark v as visited
                parent[v] = u; // Set u as the parent of v
                q.push(v); // Push v onto the queue for further exploration
            }
        }
    }

    // Reconstruct the shortest path from destination to source using parent array
    vector<int> path; // Create a vector to store the vertices of the shortest path
    for (int v = dest; v != -1; v = parent[v]) { // Traverse the parent array from destination to source
        path.push_back(v); // Add the current vertex to the path
    }
    // Reverse the path to get the shortest path from source to destination
    reverse(path.begin(), path.end()); // Reverse the order of vertices in the path vector

    return path; // Return the shortest path
}

// Function to handle client requests
void handleClient(int clientSocket, const Graph& graph, unordered_map<string, vector<int>>& cache, mutex& cacheMutex) {
    // Receive pair of vertices from client
    int src, dest;
    recv(clientSocket, &src, sizeof(src), 0); // Receive source vertex from client
    recv(clientSocket, &dest, sizeof(dest), 0); // Receive destination vertex from client

    // Check if request is already cached
    string request = to_string(src) + "-" + to_string(dest); // Create a unique identifier for the request
    vector<int> cachedPath; // Vector to store cached path
    bool isCached = false; // Flag indicating whether the request is cached or not

    {
        lock_guard<mutex> lock(cacheMutex); // Lock the mutex to access the cache safely
        auto it = cache.find(request); // Search for the request in the cache
        if (it != cache.end()) { // If request is found in the cache
            cachedPath = it->second; // Get the cached path
            isCached = true; // Set the flag indicating that the request is cached
        }
    }

    // Compute shortest path or retrieve from cache
    vector<int> path; // Vector to store the computed or retrieved path
    if (!isCached) { // If the request is not cached
        path = shortestPath(graph, src, dest); // Compute the shortest path using BFS algorithm

        // Cache the response
        {
            lock_guard<mutex> lock(cacheMutex); // Lock the mutex to access the cache safely
            if (cache.size() >= 10) { // If the cache size exceeds the maximum allowed size
                cache.erase(cache.begin()); // Remove the oldest entry from the cache
            }
            cache[request] = path; // Cache the computed path
        }
    } else { // If the request is cached
        path = cachedPath; // Retrieve the cached path
    }

    // Send shortest path to client
    int pathSize = path.size(); // Get the size of the path
    send(clientSocket, &pathSize, sizeof(pathSize), 0); // Send the size of the path to the client
    for (int v : path) { // Iterate over each vertex in the path
        send(clientSocket, &v, sizeof(v), 0); // Send each vertex to the client
    }

    close(clientSocket); // Close the client socket
}

// Main server function
void server(const string& filename, int port) {
    // Parse graph from file
    Graph graph = parseGraphFile(filename); // Parse the graph from the specified file

    // Cache for storing last 10 requests and responses
    unordered_map<string, vector<int>> cache; // Initialize an unordered_map to store cached paths
    mutex cacheMutex; // Initialize a mutex to ensure thread-safe access to the cache

    // Create TCP/IP socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (serverSocket < 0) { // Check if socket creation failed
        cerr << "Error: Unable to create socket" << endl; // Print error message
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Bind socket to address and port
    struct sockaddr_in serverAddr; // Structure to hold server address information
    memset(&serverAddr, 0, sizeof(serverAddr)); // Clear the serverAddr structure
    serverAddr.sin_family = AF_INET; // Set address family to IPv4
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Allow connections from any IP address
    serverAddr.sin_port = htons(port); // Set the port number

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) { // Bind the socket to the address and port
        cerr << "Error: Unable to bind socket to port" << endl; // Print error message
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) < 0) { // Listen for incoming connections with a queue size of 10
        cerr << "Error: Unable to listen on socket" << endl; // Print error message
        exit(EXIT_FAILURE); // Exit the program with failure status
    }

    cout << "Server listening on port " << port << endl; // Print message indicating server is listening

    // Accept incoming connections and handle them in separate threads
    while (true) { // Continuously accept connections
        struct sockaddr_in clientAddr; // Structure to hold client address information
        socklen_t clientAddrLen = sizeof(clientAddr); // Length of client address structure
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen); // Accept incoming connection
        if (clientSocket < 0) { // Check if connection acceptance failed
            cerr << "Error: Unable to accept incoming connection" << endl; // Print error message
            continue; // Continue to next iteration
        }

        cout << "New client connected" << endl; // Print message indicating new client connection

        // Spawn a new thread to handle the client request
        thread clientThread(handleClient, clientSocket, ref(graph), ref(cache), ref(cacheMutex)); // Create a new thread
        clientThread.detach(); // Detach the thread so it runs independently
    }

    // Close server socket
    close(serverSocket); // Close the server socket
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <filename> <port>" << endl;
        return EXIT_FAILURE;
    }

    string filename = argv[1];
    int port = atoi(argv[2]);

    // Start server
    server(filename, port);

    return EXIT_SUCCESS;
}

