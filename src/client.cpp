#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>
#include <unordered_map>
#include <arpa/inet.h>
#include <chrono>

#include "../include/parser/parser.h"
#include "../include/thread_pool/thread_pool.h"

constexpr int PORT = 9090;

void check(int rc, const char* msg) {
    if (rc < 0) {
        throw std::runtime_error(std::string(msg) + ": " + strerror(errno));
    }
}

void make_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw std::runtime_error("fcntl(F_GETFL) failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl(F_SETFL) failed");
    }
}

void send_client_logon(int clientSocket, sockaddr_in &clientAddress) {
    socklen_t clientAddressSize = sizeof(clientAddress);
    getsockname(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
    int port = ntohs(clientAddress.sin_port);

    std::string sender_id_field = "49=";
    sender_id_field += ip;
    sender_id_field += ":";
    sender_id_field += std::to_string(port);

    std::string fixLogon;

    fixLogon += "8=FIX.4.4";            fixLogon += SOH; // BeginString
    fixLogon += "9=65";                 fixLogon += SOH; // BodyLength (dummy)
    fixLogon += "35=A";                 fixLogon += SOH; // MsgType = Logon
    fixLogon += "34=1";                 fixLogon += SOH; // MsgSeqNum
    fixLogon += sender_id_field;        fixLogon += SOH; // SenderCompID
    fixLogon += "56=SERVER";            fixLogon += SOH; // TargetCompID
    fixLogon += "108=30";               fixLogon += SOH; // HeartBtInt = 30 sec
    fixLogon += "10=123";               fixLogon += SOH; // CheckSum (dummy)

    send(clientSocket, fixLogon.c_str(), fixLogon.size(), 0);
}

ssize_t send_client_heartbeat(int clientSocket, sockaddr_in &clientAddress) {
    socklen_t clientAddressSize = sizeof(clientAddress);
    getsockname(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
    int port = ntohs(clientAddress.sin_port);

    std::string sender_id_field = "49=";
    sender_id_field += ip;
    sender_id_field += ":";
    sender_id_field += std::to_string(port);

    std::string fixHeartbeat;
    fixHeartbeat += "8=FIX.4.4"; fixHeartbeat += '\x01';
    fixHeartbeat += "9=65";                 fixHeartbeat += SOH; // BodyLength (dummy)
    fixHeartbeat += "35=0";      fixHeartbeat += '\x01'; // MsgType=Heartbeat
    fixHeartbeat += "34=...";    fixHeartbeat += '\x01'; // MsgSeqNum
    fixHeartbeat += sender_id_field; fixHeartbeat += '\x01';
    fixHeartbeat += "56=SERVER"; fixHeartbeat += '\x01';
    fixHeartbeat += "10=123";    fixHeartbeat += '\x01'; // Checksum dummy

    return send(clientSocket, fixHeartbeat.c_str(), fixHeartbeat.size(), 0);
}

ssize_t send_client_test_req(int clientSocket, sockaddr_in &clientAddress) {
    socklen_t clientAddressSize = sizeof(clientAddress);
    getsockname(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
    int port = ntohs(clientAddress.sin_port);

    std::string sender_id_field = "49=";
    sender_id_field += ip;
    sender_id_field += ":";
    sender_id_field += std::to_string(port);

    std::string fixTestRequest;
    fixTestRequest += "8=FIX.4.4";  fixTestRequest += '\x01';   // BeginString
    fixTestRequest += "9=65";       fixTestRequest += '\x01';   // BodyLength (dummy)
    fixTestRequest += "35=1";       fixTestRequest += '\x01';   // MsgType = TestRequest
    fixTestRequest += "34=...";     fixTestRequest += '\x01';   // MsgSeqNum (incrementat de tine)
    fixTestRequest += sender_id_field; fixTestRequest += '\x01'; // SenderCompID
    fixTestRequest += "56=SERVER"; fixTestRequest += '\x01'; // TargetCompID
    fixTestRequest += "112=TEST1";  fixTestRequest += '\x01';   // TestReqID (orice string unic)
    fixTestRequest += "10=123";     fixTestRequest += '\x01';   // CheckSum (dummy)

    return send(clientSocket, fixTestRequest.c_str(), fixTestRequest.size(), 0);
}

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    check(clientSocket, "clientSocket");
    make_nonblocking(clientSocket);

    sockaddr_in clientAddress{};
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(PORT);
    clientAddress.sin_addr.s_addr = INADDR_ANY;

    int rc = connect(clientSocket, (struct sockaddr*)&clientAddress, sizeof(clientAddress));
    if (rc < 0 && errno != EINPROGRESS) {
        throw std::runtime_error(std::string("connect: ") + strerror(errno));
    }

    int epollFd = epoll_create1(0);
    check(epollFd, "epollFd");

    epoll_event ev{}, events[2];
    
    ev.events = EPOLLIN;
    ev.data.fd = clientSocket;
    check(epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &ev), "epoll_ctl socket");

    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    check(epoll_ctl(epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &ev), "epoll_ctl stdin");

    send_client_logon(clientSocket, clientAddress);

    std::chrono::steady_clock::time_point lastActivityFromClient = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastActivityFromServer = std::chrono::steady_clock::time_point{};

    bool sent_test_req = false;

    ThreadPool pool{2};

    // pool.enqueue([&] {
    //     while (true) {
    //         std::this_thread::sleep_for(std::chrono::seconds(2));

    //         auto now = std::chrono::steady_clock::now();
    //         auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivityFromClient);

    //         if (duration.count() >= 10) {
    //             send_client_heartbeat(clientSocket, clientAddress);
    //             lastActivityFromClient = std::chrono::steady_clock::now();
    //         }
    //     }
    // });

    // pool.enqueue([&] {
    //     while (true) {
    //         std::this_thread::sleep_for(std::chrono::seconds(2));

    //         auto now = std::chrono::steady_clock::now();
    //         auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivityFromServer);

    //         if (duration.count() >= 10 && sent_test_req == false) {
    //             send_client_test_req(clientSocket, clientAddress);
    //             lastActivityFromClient = std::chrono::steady_clock::now();
    //             sent_test_req = true;
    //         } else if (duration.count() >= 10 && sent_test_req ==true) {
    //             close(clientSocket);
    //             std::cout << "Server did not respond, closing client..." << std::endl;
    //             exit(0);
    //         }
    //     }
    // });
    
    while (true) {
        int nfds = epoll_wait(epollFd, events, 2, -1);
        check(nfds, "epoll_wait");

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == clientSocket) {
                char buffer[1024];
                ssize_t n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    lastActivityFromServer = std::chrono::steady_clock::now();

                    buffer[n] = '\0';
                    std::cout << "Server: " << buffer << std::endl;

                    std::string fix_msg = buffer;
                    for (auto &ch : fix_msg) {
                        if (ch == '|') {
                            ch = '\x01';
                        }
                    }

                    std::ostringstream oss;

                    auto data_opt = parse_fix_message(fix_msg);

                    if (!data_opt) {
                        oss << "Invalid fix msg!\n";
                    } else {
                        fix_data data = *data_opt;
                        for (auto key : data.keys) {
                            oss << key << " " << tag_names[key] << " " << data.fields[key] << std::endl;
                        }
                    }

                    std::string my_buff = oss.str();
                    std::cout << my_buff << std::endl;

                    fix_data data = *data_opt;
                    if (data.fields[35] == "0" && sent_test_req == true) {
                        sent_test_req = false;
                    }
                    if (data.fields[35] != "0" && sent_test_req == true) {
                        close(clientSocket);
                        std::cout << "Server did not respond, closing client..." << std::endl;
                        exit(0);
                    }
                    if (data.fields[35] == "A") {
                        //received server logon
                        printf("FIX connection established\n");
                    }
                    if (data.fields[35] == "1") {
                        send_client_heartbeat(clientSocket, clientAddress);
                        lastActivityFromClient = std::chrono::steady_clock::now();
                    }

                } else if (n == 0) {
                    std::cout << "Server disconnected\n";
                    close(clientSocket);
                    return 0;
                }
            } else if (events[i].data.fd == STDIN_FILENO) {
                std::string input;
                std::getline(std::cin, input);
                send(clientSocket, input.c_str(), input.size(), 0);
                lastActivityFromClient = std::chrono::steady_clock::now();
            }
        }
    }

    return 0;
}
