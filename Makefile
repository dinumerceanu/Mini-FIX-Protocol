CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude

SRC_DIR := src
PARSER_DIR := include/parser
SESSION_DIR := include/session_layer
THREAD_POOL_DIR := include/thread_pool
ORDER_EXECUTION_DIR := include/order_execution
UTILS_DIR := include/utils
BUILD_DIR := build

# Source files for server
SERVER_SRCS := $(SRC_DIR)/server.cpp \
               $(PARSER_DIR)/parser.cpp \
               $(SESSION_DIR)/session_layer.cpp \
               $(THREAD_POOL_DIR)/thread_pool.cpp \
               $(ORDER_EXECUTION_DIR)/order_execution.cpp \
               $(UTILS_DIR)/utils.cpp

# Source files for client
CLIENT_SRCS := $(SRC_DIR)/client.cpp \
               $(PARSER_DIR)/parser.cpp \
               $(SESSION_DIR)/session_layer.cpp \
               $(THREAD_POOL_DIR)/thread_pool.cpp \
               $(ORDER_EXECUTION_DIR)/order_execution.cpp \
               $(UTILS_DIR)/utils.cpp

# Corresponding object files
SERVER_OBJS := $(SERVER_SRCS:%.cpp=$(BUILD_DIR)/%.o)
CLIENT_OBJS := $(CLIENT_SRCS:%.cpp=$(BUILD_DIR)/%.o)

# Executables
SERVER_TARGET := server
CLIENT_TARGET := client

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Link server
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Link client
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic rule to compile .cpp into .o
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run commands
run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

run-client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)
