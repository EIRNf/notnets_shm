#include <notnets/experiments/ExperimentalData.h>
#include <notnets/experiments/Experiments.h>
#include <notnets/util/AutoTimer.h>
#include <notnets/util/RunningStat.h>
#include <notnets/util/bench_experiment_utils.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>
#include <random>
#include <vector>

// Notnets imports
#include "rpc.h"
#include "coord.h"

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

#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()

#include <assert.h> //assert

using namespace std;
using namespace notnets;
using namespace notnets::experiments;

#define SA struct sockaddr
#define PORT 8080

rtt_steady_state_tcp_experiment::rtt_steady_state_tcp_experiment() {}

void rtt_steady_state_tcp_experiment::setUp()
{
  cout << " rtt_steady_state_tcp_experiment::setUp()..." << endl;
}

void rtt_steady_state_tcp_experiment::tearDown()
{
  cout << " rtt_steady_state_tcp_experiment::tearDown()..." << endl;
}

void rtt_steady_state_tcp_experiment::make_rtt_steady_state_tcp_experiment(ExperimentalData *exp)
{
  cout << " rtt_steady_state_tcp_experiment::make_rtt_steady_state_tcp_experiment()..." << endl;
  exp->setDescription("RTT after connections have been established");
  exp->addField("queue");
  exp->addField("num_clients");
  // exp->addField("latency(ns/op)");
  exp->addField("throughput(ops/ms)");
  // exp->addField("latency_deviation");
  exp->addField("throughput_deviation");

  exp->setKeepValues(false);
}
void *rtt_steady_state_tcp_experiment::pthread_server_tcp_handler(void *arg)
{
  handler_args *args = (handler_args *)arg;

  int sockfd = *(int *)socket;

  while (!args->experiment_instance->run_flag)
    ;

  int *buf = (int *)malloc(MESSAGE_SIZE);
  int buf_size = MESSAGE_SIZE;

  int *pop_buf = (int *)malloc(MESSAGE_SIZE);
  int pop_buf_size = MESSAGE_SIZE;

  // Run messages
  for (int i = 1; i < NUM_ITEMS; i++)
  {
    read(sockfd, pop_buf, pop_buf_size);
    *buf = *pop_buf;
    write(sockfd, buf, buf_size);
  }
  close(sockfd);
  pthread_exit(0);

  args->experiment_instance = NULL;
  free(args);
  pthread_exit(0);
}

void *rtt_steady_state_tcp_experiment::pthread_client_tcp_load_connection(void *arg)
{
  int sockfd;
  struct sockaddr_in servaddr;

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    printf("socket creation failed...\n");
    exit(0);
  }
  else
    // printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);

  struct experiment_args *args = (struct experiment_args *)arg;

  char name[4];
  snprintf(name, 4, "%d", args->metrics.client_id);

  // connect the client socket to server socket
  if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
  {
    printf("connection with the server failed...\n");
    exit(0);
  }

  int *buf = (int *)malloc(MESSAGE_SIZE);
  int buf_size = MESSAGE_SIZE;

  int *pop_buf = (int *)malloc(MESSAGE_SIZE);
  int pop_buf_size = MESSAGE_SIZE;

  // hold until flag is set to true
  while (!args->experiment_instance->run_flag)
    ;

  clock_gettime(CLOCK_MONOTONIC, &args->metrics.start);
  for (int i = 1; i < args->experiment_instance->num_items; i++)
  {
    *buf = i;
    // client_send_rpc(qp, buf, buf_size);
    write(sockfd, buf, buf_size);

    // Request sent, read for response

    // client_receive_buf(qp, pop_buf, pop_buf_size);
    read(sockfd, pop_buf, pop_buf_size);

    assert(*pop_buf == *buf);
  }
  clock_gettime(CLOCK_MONOTONIC, &args->metrics.end);
  close(sockfd);
  pthread_exit(0);
}

void rtt_steady_state_tcp_experiment::process()
{

  std::cout << "rtt_steady_state_conn_experiment process" << std::endl;
  util::AutoTimer timer;

  ExperimentalData exp("rtt_steady_state_tcp_experiment");
  auto expData = {&exp};

  make_rtt_steady_state_tcp_experiment(&exp);

  for (auto exp : expData)
    exp->open();

  vector<util::RunningStat> latency;
  vector<util::RunningStat> throughput;

  // What possible queue do we have?
  // Queue Type certainly
  vector<int> queue_type =
      {1};

  for (auto __attribute__((unused)) queue : queue_type)
  {
    latency.push_back(util::RunningStat());
    throughput.push_back(util::RunningStat());
  }

  std::cout << "Running experiments..." << std::endl;

  for (auto num_clients : num_clients_)
  {
    for (int i = 0; i < numRuns_; i++)
    {
      for (auto queue : queue_type)
      {
        std::cout << "run " << i << "..." << queue << std::endl;

        int sockfd, connfd;
        socklen_t len;
        struct sockaddr_in servaddr, cli;

        // socket create and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
          printf("socket creation failed...\n");
          exit(0);
        }

        bzero(&servaddr, sizeof(servaddr));

        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(PORT);
        // Binding newly created socket to given IP and verification
        if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
        {
          printf("socket bind failed...\n");
          exit(0);
        }

        // Now server is ready to listen and verification
        if ((listen(sockfd, MAX_CLIENTS)) != 0)
        {
          printf("Listen failed...\n");
          exit(0);
        }

        len = sizeof(cli);

        // Create pointer array to keep track of client threads
        clients = new pthread_t *[num_clients];
        handlers = new pthread_t *[num_clients];

        // Spin up server
        server_context *sc = register_server((char *)"test_server_addr");

        // Create metric structs for clients
        client_args = new experiment_args *[num_clients];

        // Create client threads, will maintain a holding pattern until
        // flag is flipped
        for (int i = 0; i < num_clients; i++)
        {

          struct experiment_args *client_arg = (struct experiment_args *)malloc(sizeof(struct experiment_args));
          client_arg->experiment_instance = this;

          client_args[i] = client_arg;
          pthread_t *new_client = (pthread_t *)malloc(sizeof(pthread_t));
          clients[i] = new_client;

          client_args[i]->metrics.client_id = i * 10; //
          // atomic_thread_fence(memory_order_seq_cst);
          pthread_create(clients[i], NULL, rtt_steady_state_tcp_experiment::pthread_client_tcp_load_connection, client_args[i]);
        }

        // Keep track of client_ids to avoid repeat values
        int client_list[num_clients];

        for (int i = 0; i < num_clients;)
        {
          connfd = accept(sockfd, (SA *)&cli, &len);
          if (connfd < 0)
          {
            printf("server accept failed...\n");
            exit(0);
          }

          if (connfd)
          {
            client_list[i] = connfd;
            pthread_t *new_handler = (pthread_t *)malloc(sizeof(pthread_t));
            handlers[i] = new_handler;
            pthread_create(handlers[i], NULL, rtt_steady_state_tcp_experiment::pthread_server_tcp_handler, &client_list[i]); // server handler
            i++;
          }
        }

        // Run
        run_flag = true;

        // Join threads
        for (int i = 0; i < num_clients; i++)
        {
          pthread_join(*clients[i], NULL);
          pthread_join(*handlers[i], NULL);

          // Free heap allocated data
          // free(handlers[i]); //handled by pthreads
          free(clients[i]);
        }

        // What metrics do we care about?
        long total_ns = 0.0;
        for (int i = 0; i < num_clients; i++)
        {
          struct timespec start = client_args[i]->metrics.start;
          struct timespec end = client_args[i]->metrics.end;

          if ((end.tv_sec - start.tv_sec) > 0)
          { // we have atleast 1 sec

            long ms = (end.tv_sec - start.tv_sec) * 1.0e+03;
            ms += (end.tv_nsec - start.tv_nsec) / 1.0e+06;

            total_ns += (ms * 1.0e+06);
          }
          else
          { // nanosecond scale, probably not very accurate

            long ns = 0.0;
            ns = (end.tv_sec - start.tv_sec) * 1.0e+09;
            ns += (end.tv_nsec - start.tv_nsec);

            total_ns += ns;
          }
        }

        long average_ns = total_ns / num_clients;

        latency[queue].push(util::get_ns_op(average_ns, num_items));                      // per client average latency
        throughput[queue].push(util::rtt_get_ops_ms(average_ns, num_items, num_clients)); // total throughput

        for (int i = 0; i < num_clients; i++)
        {
          client_args[i]->experiment_instance = NULL;
          free(client_args[i]);
        }

        run_flag = false;
        shutdown(sc);
      }
    }

    for (auto queue : queue_type)
    {
      exp.addRecord();
      exp.setFieldValue("tcp", std::to_string(queue));
      exp.setFieldValue("num_clients", boost::lexical_cast<std::string>(num_clients));
      // exp.setFieldValue("latency(ns/op)", latency[queue].getMean());
      exp.setFieldValue("throughput(ops/ms)", throughput[queue].getMean());

      // exp.setFieldValue("latency_deviation", latency[queue].getStandardDeviation());
      exp.setFieldValue("throughput_deviation", latency[queue].getStandardDeviation());

      latency[queue].clear();
      throughput[queue].clear();
    }
  }

  std::cout << "done." << std::endl;

  for (auto exp : expData)
    exp->close();
};
