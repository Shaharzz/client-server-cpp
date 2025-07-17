// IDs: 207696287 - Shahar David
// 209439702 - Roi Zur
// 209439710 - Amit Zur


#include <iostream>
#include <vector>
#include <cstring> // Include cstring for memset
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// Function to connect to server and send request
void sendRequest(const char* serverIP, int port, int src, int dest) {
    // Create TCP/IP socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Error: Unable to create socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Define server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // Initialize serverAddr with zeros
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    serverAddr.sin_port = htons(port);

    // Connect to server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error: Unable to connect to server" << endl;
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Send source and destination vertices to server
    send(clientSocket, &src, sizeof(src), 0);
    send(clientSocket, &dest, sizeof(dest), 0);

    // Receive shortest path from server
    int pathSize;
    recv(clientSocket, &pathSize, sizeof(pathSize), 0);
    vector<int> path(pathSize);
    for (int i = 0; i < pathSize; ++i) {
        recv(clientSocket, &path[i], sizeof(path[i]), 0);
    }

    // Print shortest path
    cout << "Shortest path from vertex " << src << " to vertex " << dest << ": ";
    for (int v : path) {
        cout << v << " ";
    }
    cout << endl;

    // Close socket
    close(clientSocket);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <serverIP> <port> <src> <dest>" << endl;
        return EXIT_FAILURE;
    }

    const char* serverIP = argv[1];
    int port = atoi(argv[2]);
    int src = atoi(argv[3]);
    int dest = atoi(argv[4]);

    // Send request to server
    sendRequest(serverIP, port, src, dest);

    return EXIT_SUCCESS;
}

