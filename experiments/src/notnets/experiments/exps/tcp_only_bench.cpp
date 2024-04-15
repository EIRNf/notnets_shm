#include <notnets/experiments/Experiments.h>
#include <notnets/experiments/tcp_only_bench.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono/include.hpp>

#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <strings.h> // bzero()
#include <netinet/tcp.h>

#include <pthread.h>

#define MESSAGE_SIZE 4
#define SA struct sockaddr
#define PORT 8888
#define EXEC_LENGTH_SEC 300 //5 Minutes

struct connection_args
{
  int client_id;
  int num_clients;
  int run;
};

struct experiment_args
{
  struct connection_args metrics;
};

struct handler_exp_args
{
  int connfd;
  double items_processed;
};


void tcp_process()
{
  int numRuns_ = 1;
  std::vector<int> num_clients_ = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};

  for (auto num_clients : num_clients_)
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

      //Instantiate pointer arrays to manage threads and data
      pthread_t **clients;
      pthread_t **handlers;
      struct experiment_args **client_args;
      struct handler_exp_args **handler_args;

      // Create pointer array to keep track of client threads
      clients = new pthread_t *[num_clients];
      handlers = new pthread_t *[num_clients];

      // Create metric structs for clients
      client_args = new experiment_args *[num_clients];
      handler_args = new handler_exp_args *[num_clients];

      // Create client threads
      for (int i = 0; i < num_clients; i++)
      {
        struct experiment_args *client_arg = (struct experiment_args *)malloc(sizeof(struct experiment_args));

        client_args[i] = client_arg;
        client_arg->metrics.num_clients = num_clients;
        client_arg->metrics.run = numRuns;

        pthread_t *new_client = (pthread_t *)malloc(sizeof(pthread_t));
        clients[i] = new_client;

        client_args[i]->metrics.client_id = i * 10; //
        pthread_create(clients[i], NULL, pthread_client_tcp_post_connect, client_args[i]);
      }


      //ACCEPT TCP CONNECTIONS
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
          //Handlar args
          handler_exp_args *handler_arg = (handler_exp_args *)malloc(sizeof(handler_exp_args));

          handler_arg->connfd = connfd;
          handler_arg->items_processed = 0;
          handler_args[i] = handler_arg;

          pthread_t *new_handler = (pthread_t *)malloc(sizeof(pthread_t));
          handlers[i] = new_handler;
          pthread_create(handlers[i], NULL, pthread_server_tcp_handler, handler_args[i]); // server handler
          i++;
        }
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

      double total_messages = 0;
      // Count total items processed
      for (int i = 0; i < num_clients; i++)
      {
        total_messages += handler_args[i]->items_processed;
      }
      printf("total items processed : %f\n", total_messages);

      for (int i = 0; i < num_clients; i++)
      {
        free(client_args[i]);
        free(handler_args[i]);
      }

      close(sockfd);
    }
  }
}

void *pthread_server_tcp_handler(void *arg)
{
  handler_exp_args *args = (handler_exp_args *)arg;

  int sockfd = args->connfd;

  int *buf = (int *)malloc(MESSAGE_SIZE);
  int buf_size = MESSAGE_SIZE;

  int *pop_buf = (int *)malloc(MESSAGE_SIZE);
  int pop_buf_size = MESSAGE_SIZE;

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
    printf("socket creation failed...\n");
    exit(0);
  }
  else
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

  char client_file_name[17];
  snprintf(client_file_name, 17, "tcp-%d-%d-%d.txt", args->metrics.num_clients, args->metrics.client_id, args->metrics.run);

  FILE *f;
  f = fopen(client_file_name, "a+"); // a+ (create + append) option will allow appending which is useful in a log file
  if (f == NULL)
  { /* Something is wrong   */
  }
  // File Format
  // entries,send_time,return_time,
  // 1,187007185739552,187007185761008,
  fprintf(f, "entries,send_time,return_time,\n");

  auto now = boost::chrono::steady_clock::now();
  auto stop_time = now + boost::chrono::seconds{EXEC_LENGTH_SEC};
  int items_count = 0;

  while (true)
  {
    *buf = items_count;

    if (items_count % 2 != 0)
    {
      fprintf(f, "%s,", boost::lexical_cast<std::string>(items_count).c_str());
      fprintf(f, "%s,", boost::lexical_cast<std::string>(boost::chrono::steady_clock::now().time_since_epoch().count()).c_str());
    }

    write(sockfd, buf, buf_size);
    // Request sent, read for response
    read(sockfd, pop_buf, pop_buf_size);

    if (items_count % 2 != 0)
    {
      fprintf(f, "%s,", boost::lexical_cast<std::string>(boost::chrono::steady_clock::now().time_since_epoch().count()).c_str());
      fprintf(f, "\n");
    }
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
      break;
    }
  }
  close(sockfd);
  free(buf);
  free(pop_buf);
  pthread_exit(0);
}
