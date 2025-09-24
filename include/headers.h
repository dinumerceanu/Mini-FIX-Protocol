#ifndef HEADERS_H
#define HEADERS_H

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
#include <optional>
#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <future>
#include <sstream>

#include "thread_pool/thread_pool.h"
#include "parser/parser.h"
#include "session_layer/session_layer.h"
#include "order_execution/order_execution.h"
#include "utils/utils.h"

constexpr int PORT = 9090;
constexpr int PORT_OB = 8080;
constexpr int MAX_EVENTS = 100;
constexpr int BUFFER_SIZE = 1024;
constexpr char SOH = '\x01';

#endif