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

// Notnets imports
#include "rpc.h"
#include "coord.h"

using namespace std;
using namespace notnets;
using namespace notnets::experiments;

rtt_steady_state_conn_experiment::rtt_steady_state_conn_experiment() {}

void rtt_steady_state_conn_experiment::setUp()
{
  cout << " rtt_steady_state_conn_experiment::setUp()..." << endl;
}

void rtt_steady_state_conn_experiment::tearDown()
{
  cout << " rtt_steady_state_conn_experiment::tearDown()..." << endl;
}

void rtt_steady_state_conn_experiment::make_rtt_steady_state_conn_experiment(ExperimentalData *exp)
{
  cout << " rtt_steady_state_conn_experiment::make_rtt_steady_state_conn_experiment()..." << endl;
  exp->setDescription("RTT after connections have been established");

  exp->addField("queue");
  exp->addField("num_clients");
  exp->addField("throughput(ops/ms)");
  exp->addField("latency(ns/op)");
  exp->addField("latency_deviation");
  // exp->addField("throughput_deviation");

  exp->setKeepValues(false);
}

void make_notnets_client_experiment_data(ExperimentalData *exp)
{
  cout << " make_notnets_client_experiment_data()...\n" << endl;
  exp->setDescription("Per client notnet queue timestamps");
  exp->addField("entries");
  exp->addField("send_time");
  exp->addField("return_time");
  exp->setKeepValues(false);
}

void *rtt_steady_state_conn_experiment::pthread_server_rtt(void *arg)
{
  handler_exp_args *args = (handler_exp_args *)arg;
  queue_ctx *qp = (queue_ctx *)args->queue_ctx;

  int *buf = (int *)malloc(MESSAGE_SIZE);
  int buf_size = MESSAGE_SIZE;

  int *pop_buf = (int *)malloc(MESSAGE_SIZE);
  int pop_buf_size = MESSAGE_SIZE;

  // hold until flag is set to true
  while (!(args->experiment_instance->run_flag))
    ;

  args->items_processed = 0;
  while (true)
  {

    server_receive_buf(qp, pop_buf, pop_buf_size);
    *buf = *pop_buf;
    server_send_rpc(qp, buf, buf_size);
    args->items_processed++;
    if (*buf == -1)
    {
      break; // End handling
    }
  }

  free(pop_buf);
  free(buf);
  args->experiment_instance = NULL;
  free_queue_ctx(qp);
  pthread_exit(0);
}

void *rtt_steady_state_conn_experiment::pthread_connect_measure_rtt(void *arg)
{
  struct experiment_args *args = (struct experiment_args *)arg;

  char name[4];

  snprintf(name, 4, "%d", args->metrics.client_id);

  queue_ctx *c_qp = client_open(name,
                                (char *)"test_server_addr",
                                sizeof(int),
                                args->metrics.type);
  while (c_qp == NULL)
  {
    c_qp = client_open(name,
                       (char *)"test_server_addr",
                       sizeof(int),
                       args->metrics.type);
  }

  // No connection was achieved :()
  if (c_qp == NULL)
  {
    fprintf(stdout, "Missed Connection: %d \n", args->metrics.client_id);
  }
  if (c_qp->queues->request_shmaddr == NULL || c_qp->queues->response_shmaddr == NULL)
  {
    // fprintf(stdout, "Repeat Connection: %d \n", (int)hash((unsigned char*)name));
    fprintf(stdout, "Repeat Connection: %d \n", args->metrics.client_id);
    fprintf(stdout, "Client Id: %d \n", c_qp->queues->client_id);
    fprintf(stdout, "Request_Queue: %p \n", c_qp->queues->request_shmaddr);
    fprintf(stdout, "Response_Queue: %p \n", c_qp->queues->response_shmaddr);
  }

  int *buf = (int *)malloc(MESSAGE_SIZE);
  int buf_size = MESSAGE_SIZE;

  int *pop_buf = (int *)malloc(MESSAGE_SIZE);
  int pop_buf_size = MESSAGE_SIZE;

  // hold until flag is set to true
  while (!(args->experiment_instance->run_flag))
    ;
  char client_file_name[16];
  snprintf(client_file_name, 16, "%d-%d-%d-%d",args->metrics.type, args->metrics.num_clients, args->metrics.client_id, args->metrics.run);

  ExperimentalData exp(client_file_name);
  auto expData = {&exp};

  make_notnets_client_experiment_data(&exp);
  for (auto exp : expData)
    exp->csv_open();

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

    client_send_rpc(c_qp, buf, buf_size);
    // Request sent, read for response
    client_receive_buf(c_qp, pop_buf, pop_buf_size);
    exp.setFieldValueNoFlush("return_time", boost::lexical_cast<std::string>(boost::chrono::steady_clock::now().time_since_epoch().count()));

    assert(*pop_buf == *buf);
    items_count++;

    if (!(boost::chrono::steady_clock::now() < stop_time))
    {

      // Shutdown message
      *buf = -1;
      client_send_rpc(c_qp, buf, buf_size);
      // Request sent, read for response
      client_receive_buf(c_qp, pop_buf, pop_buf_size);
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
  free(pop_buf);
  free(buf);
  free_queue_ctx(c_qp);
  pthread_exit(0);
}

void rtt_steady_state_conn_experiment::process()
{

  std::cout << "rtt_steady_state_conn_experiment process" << std::endl;
  util::AutoTimer timer;

  ExperimentalData exp("rtt_steady_state_conn_experiment");
  auto expData = {&exp};

  make_rtt_steady_state_conn_experiment(&exp);

  for (auto exp : expData)
    exp->open();

  vector<util::RunningStat> latency;
  vector<util::RunningStat> throughput;
  vector<util::RunningStat> cpu;

  // What possible queue do we have?
  // Queue Type certainly
  vector<QUEUE_TYPE> queue_type =
      {
          POLL,
          BOOST,
          SEM};

  for (auto __attribute__((unused)) queue : queue_type)
  {
    latency.push_back(util::RunningStat());
    throughput.push_back(util::RunningStat());
    cpu.push_back(util::RunningStat());
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

        std::cout << "run " << numRuns << "..." << queue << std::endl;

        // Create pointer array to keep track of client threads
        clients = new pthread_t *[num_clients];
        handlers = new pthread_t *[num_clients];

        // Spin up server
        server_context *sc = register_server((char *)"test_server_addr");

        // Create metric structs for clients
        client_args = new experiment_args *[num_clients];
        handler_args = new handler_exp_args *[num_clients];

        timer.start();

        // Create client threads, will maintain a holding pattern until
        // flag is flipped
        for (int i = 0; i < num_clients; i++)
        {

          struct experiment_args *client_arg = (struct experiment_args *)malloc(sizeof(struct experiment_args));
          client_arg->experiment_instance = this;
          client_arg->metrics.type = queue;
          client_arg->metrics.log = &log;
          client_arg->metrics.num_clients = num_clients;
          client_arg->metrics.run = numRuns;

          client_args[i] = client_arg;
          pthread_t *new_client = (pthread_t *)malloc(sizeof(pthread_t));
          clients[i] = new_client;

          client_args[i]->metrics.client_id = i * 10; //
          // atomic_thread_fence(memory_order_seq_cst);
          pthread_create(clients[i], NULL, rtt_steady_state_conn_experiment::pthread_connect_measure_rtt, client_args[i]);
        }

        // Pointer to keep track of accepted queues
        queue_ctx *s_qp;

        // Keep track of client_ids to avoid repeat values
        int client_list[num_clients];
        queue_ctx *client_queues[num_clients];

        int *item;
        int key;
        for (int i = 0; i < num_clients;)
        {
          s_qp = accept(sc);
          // Check for repeat entries!!!!!!!!!
          if (s_qp != NULL)
          { // Ocasional Leak here, discover why TODO

            key = s_qp->queues->client_id;
            item = (int *)bsearch(&key, client_list, num_clients, sizeof(int), util::cmpfunc);
            // Client already accepted, reject
            if (item != NULL)
            {
              free(s_qp);
              continue;
            }

            if (s_qp->queues->request_shmaddr != NULL)
            {
              client_list[i] = s_qp->queues->client_id;
              client_queues[i] = s_qp;
              handler_exp_args *handler_arg = (handler_exp_args *)malloc(sizeof(handler_exp_args));
              handler_arg->experiment_instance = this;
              handler_arg->queue_ctx = s_qp;
              handler_arg->items_processed = 0;
              handler_args[i] = handler_arg;
              pthread_t *new_handler = (pthread_t *)malloc(sizeof(pthread_t));
              handlers[i] = new_handler;
              pthread_create(handlers[i], NULL, pthread_server_rtt, handler_args[i]);
              i++;
            }
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
          // free_queue_ctx(client_queues[i]);
          free(clients[i]);
          free(handlers[i]);
        }
        timer.stop();

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

        auto biggest_span = boost::chrono::nanoseconds(end.timestamp - start.timestamp);
        printf("biggest_span : %ld\n", biggest_span.count());

        latency[queue].push(util::get_per_client_ns_op(biggest_span.count(), total_messages, num_clients)); // per client average latency
        throughput[queue].push(util::get_ops_ms(biggest_span.count(), total_messages));                     // total throughput

        cpu[queue].push(timer.getCPUUtilization());

        for (int i = 0; i < num_clients; i++)
        {
          client_args[i]->experiment_instance = NULL;
          free(client_args[i]);
          handler_args[i]->experiment_instance = NULL;
          free(handler_args[i]);
        }

        run_flag = false;
        log.~Logger();
        shutdown(sc);
      }
    }

    for (auto queue : queue_type)
    {
      exp.addRecord();
      std::string s = getQueueName(queue);
      exp.setFieldValue("queue", s);
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
