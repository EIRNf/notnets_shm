#include <notnets/experiments/tcp_only_bench.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono/include.hpp>
#include <boost/multiprecision/cpp_int.hpp>


#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <strings.h> // bzero()
#include <netinet/tcp.h>
#include <iostream>

#include <pthread.h>
#include <stdatomic.h>

using namespace std;

// #define MESSAGE_SIZE 4
#define MESSAGE_SIZE 128

#define SA struct sockaddr
#define PORT 8888
#define EXEC_LENGTH_SEC 90 // 1:30 Minutes

struct connection_args
{
	int client_id;
	int num_clients;
	u_int items_processed;
	int run;
};

struct experiment_args
{
	struct connection_args metrics;
};

struct handler_exp_args
{
	atomic<int> connfd;
	u_int items_processed;
};

void tcp_process()
{
	int numRuns_ = 1;
	// std::vector<int> numClients = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26}; 
	std::vector<int> numClients = {1};
	//TODO NUM HANDLERS

	for (auto num_clients : numClients)
	{
		for (int numRuns = 0; numRuns < numRuns_; numRuns++)
		{
			int sockfd, connfd;
			socklen_t len;
			struct sockaddr_in servaddr, cli;

			// socket create and verification
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd == -1)
			{
				std::cerr << "Failed to open server socket";
				exit(1);
			}

			const int enable = 1;
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			{
				std::cerr << "Failed to set socket option";
				exit(1);
			}
			if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
			{
				std::cerr << "Failed to set socket option";
				exit(1);
			}

			if (setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &enable, sizeof(int)) < 0)
			{
				std::cerr << "Failed to set socket option";
				exit(1);
			}

			// if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, "reno", strlen("reno")) < 0)
			// {
			// 	perror("Failed to set socket option");
			// 	exit(1);
			// }

			bzero(&servaddr, sizeof(servaddr));

			// assign IP, PORT
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
			servaddr.sin_port = htons(PORT);
			// Binding newly created socket to given IP and verification
			if (int ret = (bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
			{
				cout << "socket bind failed..." << ret << endl;
				exit(0);
			}

			// Now server is ready to listen and verification
			if ((listen(sockfd, num_clients)) != 0)
			{
				cout << "Listen failed..." << endl;
				exit(0);
			}

			len = sizeof(cli);

			pthread_t **clients;
			pthread_t **handlers;
			struct experiment_args **client_args;
			struct handler_exp_args **handler_args;

			//  These must be dynamically allocated to execute full
			// experiment breadth in a single instantiation of the program.
			// If statically allocated then we need to recompile and update
			// the constant value for each run configuration, as we did
			// in previous versions of this experiment.

			// Create pointer array to keep track of client threads
			clients = new pthread_t *[num_clients];
			handlers = new pthread_t *[num_clients];

			// Create metric structs for clients
			client_args = new experiment_args *[num_clients];
			handler_args = new handler_exp_args *[num_clients];

			// Preallocate handler threads
			//  TODO : SINGLE THREAD
			for (int i = 0; i < num_clients;)
			{
				// Handlar args
				handler_exp_args *handler_arg = new struct handler_exp_args;

				handler_arg->connfd = -1; // Handler threads will wait for connfd to be udpated
				handler_arg->items_processed = 0;
				handler_args[i] = handler_arg;

				pthread_t *new_handler = new pthread_t;
				handlers[i] = new_handler;
				pthread_create(handlers[i], NULL, pthread_server_tcp_handler, handler_args[i]); // server handler
				i++;
			}

			// Create client threads
			for (int i = 0; i < num_clients; i++)
			{
				struct experiment_args *client_arg = new struct experiment_args;
				client_args[i] = client_arg;
				client_arg->metrics.num_clients = num_clients;
				client_arg->metrics.run = numRuns;

				pthread_t *new_client = new pthread_t;
				clients[i] = new_client;

				client_args[i]->metrics.client_id = i * 10; //
				pthread_create(clients[i], NULL, pthread_client_tcp_post_connect, client_args[i]);
			}

			// ACCEPT TCP CONNECTIONS
			fd_set set;
			struct timeval timeout;
			FD_ZERO(&set);		  /* clear the set */
			FD_SET(sockfd, &set); /* add our file descriptor to the set */

			timeout.tv_sec = 20;
			timeout.tv_usec = 0;

			for (int i = 0; i < num_clients;)
			{
				if (1 > select(sockfd + 1, &set, NULL, NULL, &timeout))
				{
					continue;
				}
				connfd = accept(sockfd, (SA *)&cli, &len);
				if (connfd < 0)
				{
					cout << boost::format("server accept failed...\n");
					exit(0);
				}
				// give preallocated handler connfd to handle.
				handler_args[i]->connfd.store(connfd);
				i++;
			}

			// Join threads
			for (int i = 0; i < num_clients; i++)
			{
				pthread_join(*clients[i], NULL);
				pthread_join(*handlers[i], NULL);

				// Free heap allocated data
				delete (clients[i]);
				delete (handlers[i]);
			}
			delete (clients);
			delete (handlers);

			u_int client_total = 0;
			for (int i = 0; i < num_clients; i++)
			{
				client_total += client_args[i]->metrics.items_processed;
				if (client_total > 0 && client_args[i]->metrics.items_processed > INT_MAX -
																					  client_total)
				{
					cerr << "Client count overflow at:" << num_clients << endl;
					// overflow
				}
			}

			u_int server_total = 0;
			// Count total items processed
			for (int i = 0; i < num_clients; i++)
			{
				server_total += handler_args[i]->items_processed;
				if (server_total > 0 && handler_args[i]->items_processed > INT_MAX -
																			   server_total)
				{
					cerr << "Server count overflow at:" << num_clients << endl;
					// overflow
				}
			}
			if (server_total != client_total)
			{
				cerr << "Not the same number of values processed!" << endl;
			}

			cout << "server total items processed " << server_total << endl;
			cout << "client total items processed " << client_total << endl;

			for (int i = 0; i < num_clients; i++)
			{
				delete (client_args[i]);
				delete (handler_args[i]);
			}
			delete (client_args);
			delete (handler_args);

			close(sockfd);
		}
	}
}

void *pthread_server_tcp_handler(void *arg)
{
	handler_exp_args *args = (handler_exp_args *)arg;

	// Client has not been created yet.
	// Wait until handler gets new connfd
	while (args->connfd.load() == -1)
	{
		usleep(500); // reduce time on processor
	}

	int sockfd = args->connfd;
	int buf_size = MESSAGE_SIZE;
	int pop_buf_size = MESSAGE_SIZE;

	boost::multiprecision::int128_t  buf, pop_buf;
	// int buf, pop_buf;
	int ret = 0;
	args->items_processed = 0;
	while (true)
	{
		ret = read(sockfd, &pop_buf, pop_buf_size);
		if (ret == -1)
		{
			cout << "client read err\n"
				 << endl;
			exit(0);
		}
		buf = pop_buf;
		ret = write(sockfd, &buf, buf_size);
		if (ret == -1)
		{
			cout << "client write err\n"
				 << endl;
			exit(0);
		}
		if (buf == -1)
		{
			break; // End handling
		}
		args->items_processed++;
		if (args->items_processed > 0 && 1 > UINT_MAX - args->items_processed)
		{
			// overflow
			cerr << "Item overflow at handler for connfd: " << args->connfd << endl;
		}
	}

	close(sockfd);
	pthread_exit(0);
}

void *pthread_client_tcp_post_connect(void *arg)
{
	int sockfd;
	struct sockaddr_in servaddr;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		cout << "socket creation failed...\n"
			 << endl;
		exit(0);
	}
	bzero(&servaddr, sizeof(servaddr));

	struct experiment_args *args = (struct experiment_args *)arg;

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(PORT);

	// connect the client socket to server socket
	while (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
	{
		cout << "connection with the server failed...\n"
			 << endl;
		exit(0);
	}


	// char buf[128] = "This is a test string";
	int buf_size = MESSAGE_SIZE;
	int pop_buf_size = MESSAGE_SIZE;
	boost::multiprecision::int128_t  buf, pop_buf;
	// int buf, pop_buf;
	int ret = 0;

	char client_file_name[17];
	snprintf(client_file_name, 17, "tcp-%d-%d-%d.csv", args->metrics.num_clients, args->metrics.client_id, args->metrics.run);

	// FILE *f;
	// f = fopen(client_file_name, "a+"); // a+ (create + append) option will allow appending which is useful in a log file
	// if (f == NULL)
	// { /* Something is wrong   */
	// 	cerr << "Failed to open log file" << endl;
	// }

	// File Format
	// entries,send_time,return_time,
	// 1,187007185739552,187007185761008,
	// fprintf(f, "entries,send_time,return_time,\n");

	auto now = boost::chrono::steady_clock::now();
	auto stop_time = now + boost::chrono::seconds{EXEC_LENGTH_SEC};
	args->metrics.items_processed = 0;
	while (true)
	{
		buf =  args->metrics.items_processed;

		// fprintf(f, "%s,", boost::lexical_cast<std::string>( args->metrics.items_processed).c_str());
		// fprintf(f, "%s,", boost::lexical_cast<std::string>(boost::chrono::steady_clock::now().time_since_epoch().count()).c_str());

		// Write message
		ret = write(sockfd, &buf, buf_size);
		if (ret == -1)
		{
			cout << "client write err\n"
				 << endl;
			exit(0);
		}
		// Request sent, read for response
		ret = read(sockfd, &pop_buf, pop_buf_size);
		if (ret == -1)
		{
			cout << "client read err\n"
				 << endl;
			exit(0);
		}
		// fprintf(f, "%s,", boost::lexical_cast<std::string>(boost::chrono::steady_clock::now().time_since_epoch().count()).c_str());
		// fprintf(f, "\n");

		assert(pop_buf == buf);
		args->metrics.items_processed++;

		if (args->metrics.items_processed > 0 && 1 > UINT_MAX - args->metrics.items_processed)
		{
			// overflow
			cerr << "Item overflow for client:" << args->metrics.client_id << ",for run:" << args->metrics.run << endl;
		}

		if (!(boost::chrono::steady_clock::now() < stop_time))
		{
			// Shutdown message
			buf = -1;
			write(sockfd, &buf, buf_size);
			// Request sent, read for response
			read(sockfd, &pop_buf, pop_buf_size);
			assert(pop_buf == buf);
			break;
		}
	}
	close(sockfd);
	pthread_exit(0);
}
