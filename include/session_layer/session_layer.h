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

// Sends Logon from server to client
ssize_t send_server_logon(Client &client);

// Sends Heartbeat from server to client
ssize_t send_server_heartbeat(Client &client);

// Sends TestRequest from server to client
ssize_t send_server_test_req(Client &client);

// Sends Logon from client to server
void send_client_logon(int clientSocket, sockaddr_in &clientAddress);

// Sends Heartbeat from client to server
ssize_t send_client_heartbeat(int clientSocket, sockaddr_in &clientAddress);

// Sends TestRequest from client to server
ssize_t send_client_test_req(int clientSocket, sockaddr_in &clientAddress);

#endif
