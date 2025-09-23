CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude

SRC_DIR := src
PARSER_DIR := include/parser
THREAD_POOL_DIR := include/thread_pool
BUILD_DIR := build

SRCS := $(SRC_DIR)/server.cpp \
        $(PARSER_DIR)/parser.cpp \
        $(THREAD_POOL_DIR)/thread_pool.cpp

OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

TARGET := server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
