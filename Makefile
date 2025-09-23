CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude

SRC_DIR := src
PARSER_DIR := include/parser
THREAD_POOL_DIR := include/thread_pool
BUILD_DIR := build

# Surse pentru server și client
SERVER_SRCS := $(SRC_DIR)/server.cpp \
               $(PARSER_DIR)/parser.cpp \
               $(THREAD_POOL_DIR)/thread_pool.cpp

CLIENT_SRCS := $(SRC_DIR)/client.cpp \
               $(PARSER_DIR)/parser.cpp \
               $(THREAD_POOL_DIR)/thread_pool.cpp

# Obiecte corespunzătoare
SERVER_OBJS := $(SERVER_SRCS:%.cpp=$(BUILD_DIR)/%.o)
CLIENT_OBJS := $(CLIENT_SRCS:%.cpp=$(BUILD_DIR)/%.o)

# Executabile
SERVER_TARGET := server
CLIENT_TARGET := client

# Ținta implicită
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Link server
$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Link client
$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regula generică de compilare pentru .o
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Comenzi de rulare
run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

run-client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)
