#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <tcp.h>
#include <inet.h>

const int PORT = 8888;
const int MESSAGE_SIZE = 128;
const int EXEC_LENGTH_SEC = 90;

void runServer()
{
    int server_fd, new_socket;
    struct sockaddr_in servaddr, cli;
    int addrlen = sizeof(servaddr);

    char buffer[MESSAGE_SIZE] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }

    const int enable = 1;
    // Allow the server to reuse the addr
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)))
    {
        std::cerr << "Setsockopt failed" << std::endl;
        return;
    }
    // Allow the server to reuse the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
    {
        std::cerr << "Setsockopt failed" << std::endl;
        return;
    }

    if (setsockopt(server_fd, IPPROTO_TCP, TCP_QUICKACK, &enable, sizeof(int)) < 0)
    {
        std::cerr << "Setsockopt failed" << std::endl;
        return;
    }

    // Set up the address struct, IP/PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
    if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "Bind failed" << std::endl;
        return;
    }

    if (listen(server_fd, 1) < 0)
    {
        std::cerr << "Listen failed" << std::endl;
        return;
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *)&servaddr, (socklen_t *)&addrlen)) < 0)
    {
        std::cerr << "Accept failed" << std::endl;
        return;
    }

    // bounceback loop
    while (true)
    {
        memset(buffer, 0, MESSAGE_SIZE);
        int bytes_read = read(new_socket, buffer, MESSAGE_SIZE);
        if (bytes_read <= 0)
        {
            break;
        }
        // Echo the received message back to the client
        write(new_socket, buffer, bytes_read);
    }

    // Close the socket
    close(new_socket);
    close(server_fd);
}

void runClient(const std::string &host)
{
    int client_fd;
    struct sockaddr_in servaddr;
    char buffer[MESSAGE_SIZE] = "hello, server!";

    // Create a client socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Socket creation failed" << std::endl;
        return;
    }

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(PORT);


    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "Connection to server failed" << std::endl;
        return;
    }

    // Write-read loop
    while (true)
    {
        // Start measuring time
        auto start = std::chrono::high_resolution_clock::now();

        // Send data to the server
        send(client_fd, buffer, strlen(buffer), 0);

        // Receive echoed data from the server
        int bytes_read = recv(client_fd, buffer, MESSAGE_SIZE, 0);
        if (bytes_read <= 0)
        {
            break;
        }

        // Calculate latency
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> latency = end - start;

        // Display the latency
        std::cout << "Latency: " << latency.count() << " ms" << std::endl;

        // Pause for a moment to avoid excessive messages
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Close the socket
    close(client_fd);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <server|client> [host]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server")
    {
        runServer();
    }
    else if (mode == "client")
    {
        if (argc < 3)
        {
            std::cerr << "Host must be specified for client mode" << std::endl;
            return 1;
        }
        std::string host = argv[2];
        runClient(host);
    }
    else
    {
        std::cerr << "Invalid mode: " << mode << std::endl;
        return 1;
    }

    return 0;
}
