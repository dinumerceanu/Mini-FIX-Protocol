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
};

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
                        clients[clientSocket] = Client{clientSocket, ipStr, clientPort, "", ""};
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
                            // clients[fd].writeBuffer += msg;

                            pool.enqueue([=, &clients] {
                                // std::this_thread::sleep_for(std::chrono::seconds(2));
                                printf("Received from %s:%d (fd=%d): %s\n", clients[fd].ipStr.c_str(), clients[fd].port, fd, msg.c_str());

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

                                clients[fd].writeBuffer += my_buff;

                                Client &client = clients[fd];
                                while (!client.writeBuffer.empty()) {
                                    ssize_t sent = send(fd, client.writeBuffer.c_str(), client.writeBuffer.size(), 0);
                                    if (sent > 0) {
                                        client.writeBuffer.erase(0, sent);
                                        if (!client.writeBuffer.empty()) {
                                            epoll_event modEv{};
                                            modEv.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR;
                                            modEv.data.fd = fd;
                                            epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &modEv);
                                        }
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
