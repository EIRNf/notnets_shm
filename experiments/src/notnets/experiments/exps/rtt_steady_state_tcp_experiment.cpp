
#include <notnets/experiments/ExperimentalData.h>
#include <notnets/experiments/Experiments.h>
#include <notnets/util/AutoTimer.h>
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

void rtt_steady_state_tcp_experiment::make_rtt_steady_state_tcp_experiment(ExperimentalData *exp)
{
  cout << " rtt_steady_state_tcp_experiment::make_rtt_steady_state_tcp_experiment()..." << endl;
  exp->setDescription("RTT after connections have been established");
  exp->addField("tcp");
  exp->addField("num_clients");
  exp->addField("throughput(ops/ms)");
  exp->addField("latency(ns/op)");
  exp->addField("latency_deviation");
  // exp->addField("throughput_deviation");

  exp->setKeepValues(false);
}
void *rtt_steady_state_tcp_experiment::pthread_server_tcp_handler(void *arg)
{
  handler_exp_args *args = (handler_exp_args *)arg;

  int sockfd = args->connfd;

  int *buf = (int *)malloc(MESSAGE_SIZE);
  int buf_size = MESSAGE_SIZE;

  int *pop_buf = (int *)malloc(MESSAGE_SIZE);
  int pop_buf_size = MESSAGE_SIZE;

  while (!args->experiment_instance->run_flag)
    ;

  args->items_processed = 0;
  while (true)
  {
    read(sockfd, pop_buf, pop_buf_size);
    *buf = *pop_buf;
    write(sockfd, buf, buf_size);
    args->items_processed++;
    if (*buf == -1)
    {
      break; // End handling
    }
  }

  close(sockfd);
  args->experiment_instance = NULL;
  pthread_exit(0);
}

void make_tcp_client_experiment_data(ExperimentalData *exp)
{
  cout << " make_tcp_client_experiment_data()..." << endl;
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
    printf("socket creation failed...\n");
    exit(0);
  }
  else
    // printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
  struct experiment_args *args = (struct experiment_args *)arg;

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);

  // connect the client socket to server socket
  while (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
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

  char client_file_name[16];
  snprintf(client_file_name, 16, "tcp-%d-%d-%d", args->metrics.num_clients, args->metrics.client_id, args->metrics.run);

  ExperimentalData exp(client_file_name);
  auto expData = {&exp};

  make_tcp_client_experiment_data(&exp);
  for (auto exp : expData)
    exp->open();

  auto now = boost::chrono::steady_clock::now();
  auto stop_time = now + boost::chrono::seconds{execution_length_seconds};
  bool first_message = true;
  bool last_message = true;
  int items_count = 0;
  while (true)
  {
    *buf = items_count;

    if (first_message)
    {
      args->metrics.log->Log("client_sent:", (unsigned long)*buf, args->metrics.client_id);
      first_message = false;
    }
    exp.addRecord();
    exp.setFieldValueNoFlush("entries", boost::lexical_cast<std::string>(items_count));
    exp.setFieldValueNoFlush("send_time",
                             boost::lexical_cast<std::string>(
                                 boost::chrono::steady_clock::now().time_since_epoch().count()));
    write(sockfd, buf, buf_size);
    // Request sent, read for response
    read(sockfd, pop_buf, pop_buf_size);
    exp.setFieldValueNoFlush("return_time", boost::lexical_cast<std::string>(boost::chrono::steady_clock::now().time_since_epoch().count()));
    assert(*pop_buf == *buf);
    items_count++;

    if (!(boost::chrono::steady_clock::now() < stop_time))
    {

      // Shutdown message
      *buf = -1;
      write(sockfd, buf, buf_size);
      // Request sent, read for response
      read(sockfd, pop_buf, pop_buf_size);
      assert(*pop_buf == *buf);

      if (last_message)
      {
        args->metrics.log->Log("client_receive:", (unsigned long)*buf, args->metrics.client_id);
        last_message = false;
      }
      break;
    }
  }
  exp.flushExpFile();
  exp.close();
  close(sockfd);
  free(buf);
  free(pop_buf);
  pthread_exit(0);
}

void rtt_steady_state_tcp_experiment::process()
{

  std::cout << "rtt_steady_state_tcp_experiment process" << std::endl;
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
  vector<int> queue_type = {0};

  for (auto __attribute__((unused)) queue : queue_type)
  {
    latency.push_back(util::RunningStat());
    throughput.push_back(util::RunningStat());
  }

  std::cout << "Running experiments..." << std::endl;

  for (auto num_clients : num_clients_)
  {
    for (int numRuns = 0; numRuns < numRuns_; numRuns++)
    {
      for (auto queue : queue_type)
      {
        util::Logger log(24);
        log.Log("start", 0, 0);
        // Reset log?

        // usleep(1000);
        std::cout << "run " << numRuns << "..." << queue << std::endl;

        int sockfd, connfd;
        socklen_t len;
        struct sockaddr_in servaddr, cli;

        // socket create and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
          perror("Failed to open server socket");
          exit(1);
        }

        const int enable = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        {
          perror("Failed to set socket option");
          exit(1);
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
        {
          perror("Failed to set socket option");
          exit(1);
        }
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
        {
          perror("Failed to set socket option");
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

          printf("socket bind failed...%d\n", ret);
          exit(0);
        }

        // Now server is ready to listen and verification
        if ((listen(sockfd, num_clients)) != 0)
        {
          printf("Listen failed...\n");
          exit(0);
        }

        len = sizeof(cli);

        // Create pointer array to keep track of client threads
        clients = new pthread_t *[num_clients];
        handlers = new pthread_t *[num_clients];

        // Create metric structs for clients
        client_args = new experiment_args *[num_clients];
        handler_args = new handler_exp_args *[num_clients];

        // Create client threads, will maintain a holding pattern until
        // flag is flipped
        for (int i = 0; i < num_clients; i++)
        {

          struct experiment_args *client_arg = (struct experiment_args *)malloc(sizeof(struct experiment_args));
          client_arg->experiment_instance = this;
          client_args[i] = client_arg;
          client_arg->metrics.log = &log;
          client_arg->metrics.num_clients = num_clients;
          client_arg->metrics.run = numRuns;

          pthread_t *new_client = (pthread_t *)malloc(sizeof(pthread_t));
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
          if (select(sockfd + 1, &set, NULL, NULL, &timeout))
          {
            connfd = accept(sockfd, (SA *)&cli, &len);
            if (connfd < 0)
            {
              printf("server accept failed...\n");
              exit(0);
            }
          }
          else
          {
            continue;
          }

          if (connfd)
          {
            handler_exp_args *handler_arg = (handler_exp_args *)malloc(sizeof(handler_exp_args));
            handler_arg->experiment_instance = this;
            handler_arg->connfd = connfd;
            handler_arg->items_processed = 0;
            handler_args[i] = handler_arg;
            pthread_t *new_handler = (pthread_t *)malloc(sizeof(pthread_t));
            handlers[i] = new_handler;
            pthread_create(handlers[i], NULL, rtt_steady_state_tcp_experiment::pthread_server_tcp_handler, handler_args[i]); // server handler
            i++;
          }
        }
        usleep(500);

        // Run
        run_flag = true;

        // Join threads
        for (int i = 0; i < num_clients; i++)
        {
          pthread_join(*clients[i], NULL);
          pthread_join(*handlers[i], NULL);

          // Free heap allocated data
          free(clients[i]);
          free(handlers[i]);
        }

        double total_messages = 0;
        // Count total items processed
        for (int i = 0; i < num_clients; i++)
        {
          printf("items processed: %f\n", handler_args[i]->items_processed);
          total_messages += handler_args[i]->items_processed;
        }
        printf("total items processed : %f\n", total_messages);

        log.ReadLogEvent(0);
        notnets::util::Logger::Event start = log.ReadLogEvent(1);           // This should be the first timestamp
        notnets::util::Logger::Event end = log.ReadLogEvent(log.g_pos - 1); // This should be the last timestamp
        auto biggest_span = boost::chrono::duration_cast<boost::chrono::nanoseconds>(end.timestamp - start.timestamp);
        printf("biggest_span : %ld\n", biggest_span.count());

        latency[queue].push(util::get_per_client_ns_op(biggest_span.count(), total_messages, num_clients)); // per client average latency
        throughput[queue].push(util::get_ops_ms(biggest_span.count(), total_messages));                     // total throughput

        for (int i = 0; i < num_clients; i++)
        {
          client_args[i]->experiment_instance = NULL;
          free(client_args[i]);
          handler_args[i]->experiment_instance = NULL;
          free(handler_args[i]);
        }

        run_flag = false;
        log.~Logger();
        close(sockfd);
      }
    }

    for (auto queue : queue_type)
    {
      exp.addRecord();
      exp.setFieldValue("tcp", std::to_string(queue));
      exp.setFieldValue("num_clients", boost::lexical_cast<std::string>(num_clients));
      exp.setFieldValue("throughput(ops/ms)", throughput[queue].getMean());
      exp.setFieldValue("latency(ns/op)", latency[queue].getMean());
      exp.setFieldValue("latency_deviation", latency[queue].getStandardDeviation());
      // exp.setFieldValue("throughput_deviation", throughput[queue].getStandardDeviation());

      latency[queue].clear();
      throughput[queue].clear();
    }
  }

  std::cout << "done." << std::endl;

  for (auto exp : expData)
    exp->close();
};
