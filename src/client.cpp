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

#include "../include/parser/parser.h"

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

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    check(clientSocket, "clientSocket");
    make_nonblocking(clientSocket);

    sockaddr_in clientAddress{};
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(8080);
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
    
    while (true) {
        int nfds = epoll_wait(epollFd, events, 2, -1);
        check(nfds, "epoll_wait");

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == clientSocket) {
                char buffer[1024];
                ssize_t n = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
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
                } else if (n == 0) {
                    std::cout << "Server disconnected\n";
                    close(clientSocket);
                    return 0;
                }
            } else if (events[i].data.fd == STDIN_FILENO) {
                std::string input;
                std::getline(std::cin, input);
                send(clientSocket, input.c_str(), input.size(), 0);
            }
        }
    }

    return 0;
}
