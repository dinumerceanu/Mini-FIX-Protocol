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
#include <sstream>
#include <chrono>

#include "../include/thread_pool/thread_pool.h"
#include "../include/parser/parser.h"

constexpr int PORT = 8080;
constexpr int MAX_EVENTS = 100;
constexpr int BUFFER_SIZE = 1024;

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

ssize_t send_server_logon(Client &client) {
    std::string sender_id_field = "49=";
    sender_id_field += "127.0.0.1";
    sender_id_field += ":";
    sender_id_field += std::to_string(PORT);

    std::string target_id_field = "56=";
    target_id_field += client.ipStr;
    target_id_field += ":";
    target_id_field += std::to_string(client.port);

    std::string fixLogon;

    fixLogon += "8=FIX.4.4";            fixLogon += SOH; // BeginString
    fixLogon += "9=65";                 fixLogon += SOH; // BodyLength (dummy)
    fixLogon += "35=A";                 fixLogon += SOH; // MsgType = Logon
    fixLogon += "34=1";                 fixLogon += SOH; // MsgSeqNum
    fixLogon += sender_id_field;        fixLogon += SOH; // SenderCompID
    fixLogon += target_id_field;        fixLogon += SOH; // TargetCompID
    fixLogon += "108=30";               fixLogon += SOH; // HeartBtInt = 30 sec
    fixLogon += "10=123";               fixLogon += SOH; // CheckSum (dummy)

    return send(client.fd, fixLogon.c_str(), fixLogon.size(), 0);
}

ssize_t send_server_heartbeat(Client &client) {
    std::string sender_id_field = "49=";
    sender_id_field += "127.0.0.1";
    sender_id_field += ":";
    sender_id_field += std::to_string(PORT);

    std::string target_id_field = "56=";
    target_id_field += client.ipStr;
    target_id_field += ":";
    target_id_field += std::to_string(client.port);

    std::string fixHeartbeat;
    fixHeartbeat += "8=FIX.4.4"; fixHeartbeat += '\x01';
    fixHeartbeat += "9=65";                 fixHeartbeat += SOH; // BodyLength (dummy)
    fixHeartbeat += "35=0";      fixHeartbeat += '\x01'; // MsgType=Heartbeat
    fixHeartbeat += "34=...";    fixHeartbeat += '\x01'; // MsgSeqNum
    fixHeartbeat += sender_id_field; fixHeartbeat += '\x01';
    fixHeartbeat += target_id_field; fixHeartbeat += '\x01';
    fixHeartbeat += "10=123";    fixHeartbeat += '\x01'; // Checksum dummy

    return send(client.fd, fixHeartbeat.c_str(), fixHeartbeat.size(), 0);
}

ssize_t send_server_test_req(Client &client) {
    std::string sender_id_field = "49=";
    sender_id_field += "127.0.0.1";
    sender_id_field += ":";
    sender_id_field += std::to_string(PORT);

    std::string target_id_field = "56=";
    target_id_field += client.ipStr;
    target_id_field += ":";
    target_id_field += std::to_string(client.port);

    std::string fixTestRequest;
    fixTestRequest += "8=FIX.4.4";  fixTestRequest += '\x01';   // BeginString
    fixTestRequest += "9=65";       fixTestRequest += '\x01';   // BodyLength (dummy)
    fixTestRequest += "35=1";       fixTestRequest += '\x01';   // MsgType = TestRequest
    fixTestRequest += "34=...";     fixTestRequest += '\x01';   // MsgSeqNum (incrementat de tine)
    fixTestRequest += sender_id_field; fixTestRequest += '\x01'; // SenderCompID
    fixTestRequest += target_id_field; fixTestRequest += '\x01'; // TargetCompID
    fixTestRequest += "112=TEST1";  fixTestRequest += '\x01';   // TestReqID (orice string unic)
    fixTestRequest += "10=123";     fixTestRequest += '\x01';   // CheckSum (dummy)

    return send(client.fd, fixTestRequest.c_str(), fixTestRequest.size(), 0);
}

int main() {
    try {
        int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        check(serverSocket, "serverSocket");

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(8080);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        make_nonblocking(serverSocket);

        check(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)), "bind");

        check(listen(serverSocket, 100), "listen");

        int epollFd = epoll_create1(0);
        check(epollFd, "epoll_create1");

        epoll_event ev{}, events[MAX_EVENTS];
        ev.events = EPOLLIN;
        ev.data.fd = serverSocket;

        check(epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &ev), "epoll_ctl serverSocket add");

        printf("Server listening on 127.0.0.1:%d\n", PORT);

        std::unordered_map<int, Client> clients;

        ThreadPool pool{10};

        pool.enqueue([=, &clients] {
            while (true) {
                auto now = std::chrono::steady_clock::now();

                for (auto &[fd, client] : clients) {
                    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - client.lastActivityFromServer);
                    if (duration.count() >= 5) {
                        send_server_heartbeat(client);
                        client.lastActivityFromServer = now;
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });

        pool.enqueue([=, &clients] {
            while (true) {
                std::vector<int> to_remove;

                auto now = std::chrono::steady_clock::now();

                for (auto &[fd, client] : clients) {
                    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - client.lastActivityFromClient);
                    if (duration.count() >= 10 && client.sent_test_req == false) {
                        send_server_test_req(client);
                        client.lastActivityFromServer = now;
                        client.sent_test_req = true;
                    } else if (duration.count() >= 10 && client.sent_test_req == true) {
                        client.fix_conn_est = false;
                        client.lastActivityFromClient = std::chrono::steady_clock::now();
                        epoll_ctl(epollFd, EPOLL_CTL_DEL, client.fd, nullptr);
                        close(client.fd);
                        printf("Connection terminated with %s:%d (fd=%d) because no response\n", client.ipStr.c_str(), client.port, client.fd);
                        to_remove.push_back(client.fd);
                    }
                }
                
                for (int fd : to_remove) {
                    clients.erase(fd);
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        });

        while (true) {
            int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);

            for (int i = 0; i < nfds; i++) {
                int fd = events[i].data.fd;
                uint32_t evFlags = events[i].events;

                if (evFlags & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                    printf("Client %s:%d disconnected\n", clients[fd].ipStr.c_str(), clients[fd].port);
                    clients.erase(fd);
                    continue;
                }

                if (fd == serverSocket) {
                    while (true) {
                        sockaddr_in clientAddr;
                        socklen_t clientAddrLen = sizeof(clientAddr);
                        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
                        if (clientSocket < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                            break;
                        } else {
                            check(clientSocket, "clientSocket");
                        }

                        make_nonblocking(clientSocket);

                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
                        uint16_t clientPort = ntohs(clientAddr.sin_port);
                        
                        printf("New client connected from %s:%d with fd: %d\n", ipStr, clientPort, clientSocket);

                        epoll_event clientEvent{};
                        clientEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
                        clientEvent.data.fd = clientSocket;

                        epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &clientEvent);
                        clients[clientSocket] = Client{clientSocket, ipStr, clientPort, "", "", false, std::chrono::steady_clock::time_point{}, std::chrono::steady_clock::time_point{}, false};
                    }
                    continue;
                }

                if (evFlags & EPOLLIN) {
                    char buffer[BUFFER_SIZE];
                    while (true) {
                        ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
                        if (bytes > 0) {
                            std::string msg(buffer, bytes);
                            clients[fd].readBuffer += msg;
                            clients[fd].writeBuffer += msg;

                            pool.enqueue([=, &clients] {
                                // std::this_thread::sleep_for(std::chrono::seconds(2));
                                printf("Received from %s:%d (fd=%d): %s\n", clients[fd].ipStr.c_str(), clients[fd].port, fd, msg.c_str());

                                clients[fd].lastActivityFromClient = std::chrono::steady_clock::now();

                                std::string fix_msg = msg;
                                for (auto &ch : fix_msg) {
                                    if (ch == '|') {
                                        ch = '\x01';
                                    }
                                }

                                std::ostringstream oss;

                                auto data_opt = parse_fix_message(fix_msg);

                                if (!data_opt) {
                                    oss << "Invalid fix msg!\n";
                                    // return 1;
                                } else {
                                    fix_data data = *data_opt;
                                    for (auto key : data.keys) {
                                        oss << key << " " << tag_names[key] << " " << data.fields[key] << std::endl;
                                    }
                                }

                                std::string my_buff = oss.str();
                                std::cout << my_buff << std::endl;


                                Client &client = clients[fd];
                                fix_data data = *data_opt;
                                if (data.fields[35] == "A") {
                                    //received a logon
                                    int rc = send_server_logon(client);
                                    if (rc < 0) {
                                        printf("FIX connection failed with %s:%d\nClosing connection with client...\n", client.ipStr.c_str(), client.port);
                                    } else {
                                        printf("FIX connection established with %s:%d\n", client.ipStr.c_str(), client.port);
                                        client.fix_conn_est = true;
                                        client.lastActivityFromServer = std::chrono::steady_clock::now();
                                    }
                                }

                                if (data.fields[35] != "0" && client.sent_test_req == true) {
                                    client.fix_conn_est = false;
                                    client.lastActivityFromClient = std::chrono::steady_clock::now();
                                    epoll_ctl(epollFd, EPOLL_CTL_DEL, client.fd, nullptr);
                                    close(client.fd);
                                    clients.erase(client.fd);
                                    printf("Connection terminated with %s:%d (fd=%d)\n", client.ipStr.c_str(), client.port, client.fd);
                                }

                                if (data.fields[35] == "0" && client.sent_test_req == true) {
                                    client.sent_test_req = false;
                                }

                                if (data.fields[35] == "1") {
                                    send_server_heartbeat(client);
                                }

                                // Client &client = clients[fd];
                                // while (!client.writeBuffer.empty()) {
                                //     ssize_t sent = send(fd, client.writeBuffer.c_str(), client.writeBuffer.size(), 0);
                                //     if (sent > 0) {
                                //         client.writeBuffer.erase(0, sent);
                                //         if (!client.writeBuffer.empty()) {
                                //             epoll_event modEv{};
                                //             modEv.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR;
                                //             modEv.data.fd = fd;
                                //             epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &modEv);
                                //         }
                                //     } else {
                                //         if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                //             break;
                                //         } else {
                                //             epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                                //             close(fd);
                                //             clients.erase(fd);
                                //             break;
                                //         }
                                //     }
                                // }
                            });
                        } else if (bytes == 0) {
                            epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                            close(fd);
                            clients.erase(fd);
                            break;
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;
                            } else {
                                epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                                close(fd);
                                clients.erase(fd);
                                break;
                            }
                        }
                    }
                }

                if (evFlags & EPOLLOUT) {
                    Client &client = clients[fd];
                    while (!client.writeBuffer.empty()) {
                        ssize_t sent = send(fd, client.writeBuffer.c_str(), client.writeBuffer.size(), 0);
                        if (sent > 0) {
                            client.writeBuffer.erase(0, sent);
                            client.lastActivityFromServer = std::chrono::steady_clock::now();
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            else {
                                epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
                                close(fd);
                                clients.erase(fd);
                                break;
                            }
                        }
                    }

                    if (client.writeBuffer.empty()) {
                        epoll_event modEv{};
                        modEv.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
                        modEv.data.fd = fd;
                        epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &modEv);
                    }
                }
            }
        }
        close(serverSocket);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
