#pragma once




void *pthread_server_tcp_handler(void *arg);

void *pthread_client_tcp_post_connect(void *arg);
void tcp_process();