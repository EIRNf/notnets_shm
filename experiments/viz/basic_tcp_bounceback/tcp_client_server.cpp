#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <arpa/inet.h> // inet_addr()
#include <cassert>
#include <vector>
#include <numeric>
#include <limits.h>

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
    u_int items_processed = 0;
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
        if(buffer[0] == -1){
            break;
        }
    }

    // Close the socket
    close(new_socket);
    close(server_fd);
}

double average(std::vector<double> const& v){
    if(v.empty()){
        return 0;
    }

    auto const count = static_cast<double>(v.size());
    return std::reduce(v.begin(), v.end())/ count;
}

void runClient()
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

    //in micro
    std::vector<double> all_lat;
    // Write-read loop
    auto start_time = std::chrono::high_resolution_clock::now();
    auto stop_time = start_time + std::chrono::seconds{EXEC_LENGTH_SEC};
    u_int items_processed = 0;

    while (true)
    {
        // Start measuring time
        auto send_time = std::chrono::high_resolution_clock::now();

        // Send data to the server
        write(client_fd, buffer, strlen(buffer));
        // Receive echoed data from the server
        int bytes_read = read(client_fd, buffer, MESSAGE_SIZE);
        if (bytes_read <= 0)
        {
            break;
        }
        items_processed++;

        if (items_processed > 0 && 1 > UINT_MAX - items_processed)
		{
			// overflow
			std::cerr << "Item overflow for client" << std::endl;
		}
        // Calculate latency
        auto receive_time = std::chrono::high_resolution_clock::now();
        //in micro
        std::chrono::duration<double, std::milli> latency = receive_time - send_time;
        all_lat.emplace_back(latency.count());

        if (std::chrono::high_resolution_clock::now() > stop_time){
            buffer[0] = -1;
            write(client_fd, buffer, strlen(buffer));
            read(client_fd, &buffer, strlen(buffer));
            assert( -1 == (int) buffer[0]);
            break;
        }
    }

    // Display the latency
    std::cout << "Latency: " << average(all_lat) << " ms" << std::endl;
    // Display the throughput in mb/s
    // auto bytes_processed = items_processed * MESSAGE_SIZE;
    // auto mb_processed = bytes_processed / 1e+6;
    // std::cout<< "Throughput(mb/s): " << mb_processed/EXEC_LENGTH_SEC << std::endl;
    // Display the throughput in rps
    std::cout<< "Throughput(rps): " << items_processed/EXEC_LENGTH_SEC << std::endl;

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
        runClient();
    }
    else
    {
        std::cerr << "Invalid mode: " << mode << std::endl;
        return 1;
    }

    return 0;
}
