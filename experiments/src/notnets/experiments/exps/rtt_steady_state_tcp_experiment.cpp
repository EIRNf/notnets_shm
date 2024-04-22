
#include <notnets/experiments/ExperimentalData.h>
#include <notnets/experiments/Experiments.h>
#include <notnets/util/RunningStat.h>
#include <notnets/util/bench_experiment_utils.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono/include.hpp>

#include <iostream>
#include <random>
#include <vector>

// socket imports
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>

#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <strings.h> // bzero()
#include <netinet/tcp.h>

#include <assert.h> //assert
#include <regex.h>

using namespace std;
using namespace notnets;
using namespace notnets::experiments;

#define SA struct sockaddr
#define PORT 8888

rtt_steady_state_tcp_experiment::rtt_steady_state_tcp_experiment() {}

void rtt_steady_state_tcp_experiment::setUp()
{
  cout << " rtt_steady_state_tcp_experiment::setUp()..." << endl;
}

void rtt_steady_state_tcp_experiment::tearDown()
{
  cout << " rtt_steady_state_tcp_experiment::tearDown()..." << endl;
}

void *rtt_steady_state_tcp_experiment::pthread_server_tcp_handler(void *arg)
{

  handler_exp_args *args = (handler_exp_args *)arg;

  int sockfd = args->connfd;

  int buf_size = MESSAGE_SIZE;
  int pop_buf_size = MESSAGE_SIZE;

  int buf, pop_buf;
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
    args->items_processed++;
    if (buf == -1)
    {
      break; // End handling
    }
  }

  close(sockfd);
  pthread_exit(0);
}

void make_tcp_client_experiment_data(ExperimentalData *exp)
{
  // cout << " make_tcp_client_experiment_data()...\n" << endl;
  exp->setDescription("Per client TCP timestamps");
  exp->addField("entries");
  exp->addField("send_time");
  exp->addField("return_time");
  exp->setKeepValues(false);
}

void *rtt_steady_state_tcp_experiment::pthread_client_tcp_load_connection(void *arg)
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
  else
  {
    cout << "Socket successfully created..\n"
         << endl;
    bzero(&servaddr, sizeof(servaddr));
  }

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

  int buf_size = MESSAGE_SIZE;
  int pop_buf_size = MESSAGE_SIZE;
  int buf, pop_buf;

  char client_file_name[16];
  snprintf(client_file_name, 16, "tcp-%d-%d-%d", args->metrics.num_clients, args->metrics.client_id, args->metrics.run);

  ExperimentalData exp(client_file_name);
  auto expData = {&exp};

  make_tcp_client_experiment_data(&exp);
  for (auto exp : expData)
    exp->csv_open();

  auto now = boost::chrono::steady_clock::now();
  auto stop_time = now + boost::chrono::seconds{execution_length_seconds};
  int items_count = 0;
  int ret = 0;
  while (true)
  {
    buf = items_count;

    // Sample half of all messages
    if (items_count % 2 != 0)
    {
      exp.addRecord();
      exp.setFieldValueNoFlush("entries", boost::lexical_cast<std::string>(items_count));
      exp.setFieldValueNoFlush("send_time",
                               boost::lexical_cast<std::string>(
                                   boost::chrono::steady_clock::now().time_since_epoch().count()));
    }

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

    // Sample half of all messages
    if (items_count % 2 != 0)
    {
      exp.setFieldValueNoFlush("return_time", boost::lexical_cast<std::string>(boost::chrono::steady_clock::now().time_since_epoch().count()));
    }
    assert(pop_buf == buf);
    items_count++;

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
  exp.flushExpFile();
  exp.close();
  close(sockfd);
  pthread_exit(0);
}

void rtt_steady_state_tcp_experiment::process()
{

  std::cout << "rtt_steady_state_tcp_experiment process" << std::endl;

  // What possible queue do we have?
  // Queue Type certainly
  vector<int> queueType = {0};

  std::cout << "Running experiments..." << std::endl;

  for (auto num_clients : numClients)
  {
    for (int numRuns = 0; numRuns < numRuns_; numRuns++)
    {
      for (auto queue : queueType)
      {
        util::Logger log(24);
        log.Log("start", 0, 0);

        std::cout << "run " << numRuns << "..." << queue << std::endl;

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
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
        {
          std::cerr << "Failed to set socket option";
          exit(1);
        }

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

        //  These must be dynamically allocated to execute full
        // experiment breadth in a single instantiation of the program.
        // If statically allocated then we need to recompile and update
        // the constant value for each run configuration, as we did
        // in previous versions of this experiment.

        clients = new pthread_t *[num_clients];
        handlers = new pthread_t *[num_clients];

        // Create metric structs for clients
        client_args = new experiment_args *[num_clients];
        handler_args = new handler_exp_args *[num_clients];

        // Create client threads, will maintain a holding pattern until
        // flag is flipped
        for (int i = 0; i < num_clients; i++)
        {

          struct experiment_args *client_arg = new struct experiment_args;
          client_arg->experiment_instance = this;
          client_args[i] = client_arg;
          client_arg->metrics.log = &log;
          client_arg->metrics.num_clients = num_clients;
          client_arg->metrics.run = numRuns;

          pthread_t *new_client = new pthread_t;
          clients[i] = new_client;

          client_args[i]->metrics.client_id = i * 10; //
          // atomic_thread_fence(memory_order_seq_cst);
          pthread_create(clients[i], NULL, rtt_steady_state_tcp_experiment::pthread_client_tcp_load_connection, client_args[i]);
        }

        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set);        /* clear the set */
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
          handler_exp_args *handler_arg = new struct handler_exp_args;
          handler_arg->experiment_instance = this;
          handler_arg->connfd = connfd;
          handler_arg->items_processed = 0;
          handler_args[i] = handler_arg;
          pthread_t *new_handler = new pthread_t;
          handlers[i] = new_handler;
          pthread_create(handlers[i], NULL, rtt_steady_state_tcp_experiment::pthread_server_tcp_handler, handler_args[i]); // server handler
          i++;
        }

        // Join threads
        for (int i = 0; i < num_clients; i++)
        {
          pthread_join(*clients[i], NULL);
          pthread_join(*handlers[i], NULL);

          // Free heap allocated data
          free(clients[i]);
          free(handlers[i]);
        }
        free(clients);
        free(handlers);

        double total_messages = 0;
        // Count total items processed
        for (int i = 0; i < num_clients; i++)
        {
          cout << "items processed:" << handler_args[i]->items_processed << endl;
          total_messages += handler_args[i]->items_processed;
        }
        cout << "total items processed " << total_messages << endl;

        for (int i = 0; i < num_clients; i++)
        {
          free(client_args[i]);
          free(handler_args[i]);
        }
        free(client_args);
        free(handler_args);

        close(sockfd);
      }
    }
  }

  std::cout << "done." << std::endl;
};
