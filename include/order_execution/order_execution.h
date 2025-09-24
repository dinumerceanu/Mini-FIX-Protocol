#ifndef ORDER_EXECUTION_H
#define ORDER_EXECUTION_H

#include "../headers.h"

struct OrderExecutionInfo {
    std::string clOrdID;
    int cumQty;
    int leavesQty;
    int lastShares;
    double lastPx;
    bool isFilled;
    bool orderUnfilled;
};

// Create a new OrderExecutionInfo object
OrderExecutionInfo newOrderExecInfo(std::string clOrdID, int orderSize);

// Parse a single OrderBook message and update existing OrderExecutionInfo
void parse_orderbook_msg(OrderExecutionInfo &info_in, std::string &msg);

// Print OrderExecutionInfo (for debugging)
void printOrdExecInfo(OrderExecutionInfo &info);

// Generate a unique order ID (internal)
std::string generateOrderID();

// Extract order ID from OrderBook response
std::string getIdFromResp(std::string &s);

// Create a FIX Execution Report message from OrderExecutionInfo
std::string createExecutionReport(OrderExecutionInfo &info, Client &client);

#endif
