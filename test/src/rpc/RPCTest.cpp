#include "gtest/gtest.h"

#include "rpc.h"
#include "mem.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TEST_CLIENTS 10

int cmpfunc(const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

atomic_bool run_flag = false; //control execution


class RPCTest : public ::testing::Test
{
public:
    RPCTest() {}
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
protected:

};


struct test_connection_args {
    int client_id;
    queue_pair *qp;
    int err;
};


TEST_F(RPCTest, ServerShutDown)
{
  server_context* sc = register_server((char*)"common_name");
    int err = (sc == NULL);
    shutdown(sc);
    EXPECT_FALSE(err);
}

TEST_F(RPCTest, OpenClose)
{
    int status = 0;
    pid_t wpid, pid;
    pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
      server_context* sc = register_server((char*)"test_server_addr");

        while ((wpid = wait(&status)) > 0);

        shutdown(sc);
    } else { // CHILD PROCESS
        // if returns, then open was successful
      queue_pair* qp = client_open((char*)"test_client_addr",
				   (char*)"test_server_addr",
                                     sizeof(int));

        while (qp == NULL) {
	  qp = client_open((char*)"test_client_addr",
			   (char*)"test_server_addr",
                             sizeof(int));
        }

        int err = client_close((char*)"test_client_addr", (char*)"test_server_addr");
	EXPECT_FALSE(err);
        exit(0);
    }
}

TEST_F(RPCTest, Accept)
{
    // shm for communication between parent and child process
    key_t ipc_key = 1;
    int ipc_shm_size = sizeof(void*) * 2;
    int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
    void* ipc_shmaddr = shm_attach(ipc_shmid);
    void** two_ptrs = (void**) ipc_shmaddr;

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
      server_context* sc = register_server((char*)"test_server_addr");

        queue_pair* qp;
        while ((qp = accept(sc)) == NULL);

        // wait for client to update the ipc shm
        while (two_ptrs[0] == 0 && two_ptrs[0] == 0);

        // the accepted qp should be same as the qp given to the client, since
        // there should only be one reserved/allocated slot
        int err = qp->request_shmaddr != two_ptrs[0] ||
            qp->response_shmaddr != two_ptrs[1];

	EXPECT_FALSE(err);

        free(qp);
        shutdown(sc);
    } else { // CHILD PROCESS
      queue_pair* qp = client_open((char*)"test_client_addr",
				   (char*)"test_server_addr",
                                     sizeof(int));
        while (qp == NULL) {
	  qp = client_open((char*)"test_client_addr",
			   (char*)"test_server_addr",
                             sizeof(int));
        }

        two_ptrs[0] = qp->request_shmaddr;
        two_ptrs[1] = qp->response_shmaddr;

        free(qp);
        exit(0);
    }

    // cleanup ipc shm
    shm_detach(ipc_shmaddr);
    shm_remove(ipc_shmid);
}

TEST_F(RPCTest, SendRecvInt)
{
    // shm for communication between parent and child process
    key_t ipc_key = 1;
    int ipc_shm_size = sizeof(void*) * 2;
    int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
    void* ipc_shmaddr = shm_attach(ipc_shmid);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
      server_context* sc = register_server((char*)"test_server_addr");

        queue_pair* qp;
        while ((qp = accept(sc)) == NULL);

        int *buf = (int*)malloc(sizeof(int));
        int buf_size = sizeof(int);

        for (int i = 0; i < 10; ++i) {
            int pop_size = server_receive_buf(qp, buf, buf_size);
            assert(pop_size == 0);

            // do work on client rpc
            *buf *= 2;
            server_send_rpc(qp, buf, buf_size);
        }

        free(buf);
        free(qp);

        // wait for client to finish reading responses
        while (*((int*) ipc_shmaddr) != 1);

        shutdown(sc);
    } else { // CHILD PROCESS
      queue_pair* qp = client_open((char*)"test_client_addr",
                                     (char*)"test_server_addr",
                                     sizeof(int));

        while (qp == NULL) {
            qp = client_open((char*)"test_client_addr",
                             (char*)"test_server_addr",
                             sizeof(int));
        }

        // currently the shm_allocator only handles int
        int* buf = (int*)malloc(sizeof(int));
        int buf_size = sizeof(int);

        // send 1-10 to server
        for (int i = 0; i < 10; ++i) {
            *buf = i;
            client_send_rpc(qp, buf, buf_size);
        }


        int err = 0;
        // get server responses
        for (int i = 0; i < 10; ++i) {
            int pop_size = client_receive_buf(qp, buf, buf_size);
            assert(pop_size == 0);

            if (*buf != i * 2) {
                err = 1;
            }
        }
	EXPECT_FALSE(err);

        // usually we would close client conn, but server will shutdown in its
        // process, which will handle deallocation of qp
        free(buf);
        free(qp);

        *((int*) ipc_shmaddr) = 1;
        exit(0);
    }

    // cleanup ipc shm
    shm_detach(ipc_shmaddr);
    shm_remove(ipc_shmid);
}

TEST_F(RPCTest, SendRecvStr)
{
    // shm for communication between parent and child process
    key_t ipc_key = 1;
    int ipc_shm_size = sizeof(void*) * 2;
    int ipc_shmid = shm_create(ipc_key, ipc_shm_size, create_flag);
    void* ipc_shmaddr = shm_attach(ipc_shmid);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
    } else if (pid > 0) { // PARENT PROCESS
      server_context* sc = register_server((char*)"test_server_addr");

        queue_pair* qp;
        while ((qp = accept(sc)) == NULL);

        int message_size = sizeof(char[10]);
        char *buf = (char*)malloc(message_size);

        for (int i = 0; i < 10; ++i) {
            server_receive_buf(qp, buf, message_size);

            // do work on client rpc
            char response[10] = "Hi, ";
            strncat(response, buf, strlen(buf));
            server_send_rpc(qp, response, message_size);
        }

        free(buf);
        free(qp);

        // wait for client to finish reading responses
        while (*((int*) ipc_shmaddr) != 1);

        shutdown(sc);
    } else { // CHILD PROCESS
        int message_size = sizeof(char[10]);
        queue_pair* qp = client_open((char*)"test_client_addr",
                                     (char*)"test_server_addr",
                                     message_size);

        while (qp == NULL) {
            qp = client_open((char*)"test_client_addr",
                             (char*)"test_server_addr",
                             sizeof(int));
        }

        char* buf = (char*)malloc(message_size);

        for (int i = 0; i < 10; ++i) {
            snprintf(buf, 10, "%d", i);
            client_send_rpc(qp, buf, message_size);
        }


        int err = 0;
        // get server responses
        for (int i = 0; i < 10; ++i) {
            client_receive_buf(qp, buf, message_size);

            char expected[10];
            snprintf(expected, 10, "Hi, %d", i);

            if (strcmp(buf, expected) != 0) {
                err = 1;
            }
        }

	EXPECT_FALSE(err);

        // usually we would close client conn, but server will shutdown in its
        // process, which will handle deallocation of qp
        free(buf);
        free(qp);

        *((int*) ipc_shmaddr) = 1;
        exit(0);
    }

    // cleanup ipc shm
    shm_detach(ipc_shmaddr);
    shm_remove(ipc_shmid);
}

void* test_client_connection(void* arg){
    struct test_connection_args *args = (struct test_connection_args*) arg;

   
    char name[32];
    snprintf(name, 32, "%d", args->client_id);

    while(!run_flag);

    queue_pair* c_qp = client_open(name,
                                (char*)"test_server_addr",
                                 sizeof(int));
    while (c_qp == NULL) {
        c_qp = client_open(name,
                             (char*)"test_server_addr",
                             sizeof(int));
        }
    
    args->err = 0;
    args->qp = c_qp;

    // No connection was achieved :()
    if (c_qp == NULL){
        args->qp = NULL;
        args->err = 1;
        fprintf(stdout, "Missed Connection: %d \n", args->client_id);
    }
    if (c_qp->request_shmaddr == NULL || c_qp->response_shmaddr == NULL ){
        args->err = 1;
        fprintf(stdout, "Repeat Connection: %d \n", (int)hash((unsigned char*)name));
        fprintf(stdout, "Repeat Connection: %d \n", args->client_id);
        fprintf(stdout, "Client Id: %d \n", c_qp->client_id);
        fprintf(stdout, "Request_Queue: %p \n", c_qp->request_shmaddr);
        fprintf(stdout, "Response_Queue: %p \n", c_qp->response_shmaddr);
    }
    free(c_qp);
    pthread_exit(0);
}


TEST_F(RPCTest, Connection)
{
    int err = 0;

    //Clients to connect
    pthread_t *clients[TEST_CLIENTS] = {};

    //Create server
    server_context* sc = register_server((char*)"test_server_addr");


    //Args to capture client input
    struct test_connection_args *args[TEST_CLIENTS] = {};


    //Create client threads, will maintain a holding pattern until flag is flipped
    struct timespec nonce;
    for(int i = 0; i < TEST_CLIENTS; i++){
      struct test_connection_args *client_args = (struct test_connection_args *)malloc(sizeof(struct test_connection_args));
        args[i] = client_args;
        pthread_t *new_client = (pthread_t*)malloc(sizeof(pthread_t));
        clients[i] = new_client;
        if (! timespec_get(&nonce, TIME_UTC)){
            // return -1;
        }
        args[i]->client_id =  i + nonce.tv_nsec;
        atomic_thread_fence(memory_order_seq_cst);
        pthread_create(clients[i], NULL, test_client_connection, args[i]);
    }
    
    queue_pair* s_qp;

    //Synchronization variable to begin attempting to open connections
    run_flag = true;

    int client_list[TEST_CLIENTS];
    queue_pair *client_queues[TEST_CLIENTS];
    int *item;
    int key;
    for (int i=0;i < TEST_CLIENTS;){
        s_qp = accept(sc);
        if (s_qp != NULL){ 
            
            //Check for repeat entries!!!!!!!!!
            key = s_qp->client_id;
            item = (int*) bsearch(&key, client_list,TEST_CLIENTS,sizeof(int),cmpfunc);
            //Client already accepted, reject
            if(item != NULL){
                free(s_qp);
                continue;
            }

            if(s_qp->request_shmaddr != NULL){
                client_list[i] = s_qp->client_id;
                client_queues[i] = s_qp;
                i++;
            }
            // free(s_qp);
        }
    }

    atomic_thread_fence(memory_order_seq_cst);
    //Join threads
    for(int i =0; i < TEST_CLIENTS; i++){
        pthread_join(*clients[i],NULL);
        //Free heap allocated data
        free(client_queues[i]);
        free(clients[i]);
    }   



    for (int i=0; i < TEST_CLIENTS; i++){
        if (args[i]->err){
            err = args[i]->err;
        } 
        free(args[i]);
    }
    shutdown(sc);
    run_flag = false;

    EXPECT_FALSE(err);

}

