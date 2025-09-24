#ifndef SESSION_LAYER_H
#define SESSION_LAYER_H

#include "../headers.h"

struct Client {
    int fd;
    std::string ipStr;
    uint16_t port;
    std::string readBuffer;
    std::string writeBuffer;
    bool fix_conn_est;
    std::chrono::steady_clock::time_point lastActivityFromClient;
    std::chrono::steady_clock::time_point lastActivityFromServer;
    bool sent_test_req;
};

ssize_t send_server_logon(Client &client);

ssize_t send_server_heartbeat(Client &client);

ssize_t send_server_test_req(Client &client);

void send_client_logon(int clientSocket, sockaddr_in &clientAddress);

ssize_t send_client_heartbeat(int clientSocket, sockaddr_in &clientAddress);

ssize_t send_client_test_req(int clientSocket, sockaddr_in &clientAddress);

#endif
