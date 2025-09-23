#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <optional>

constexpr char SOH = '\x01';

std::unordered_map<int, std::string> tag_names = {
    {8, "BeginString"},
    {9, "BodyLength"},
    {35, "MsgType"},
    {49, "SenderCompID"},
    {56, "TargetCompID"},
    {34, "MsgSeqNum"},
    {10, "CheckSum"},
    {108, "HeartBtInt"},
    // Orders Tags
    {11, "ClOrdID"},        // Order ID
    {21, "HandlInst"},      // HandlInst (e.g., automated)
    {55, "Symbol"},         // Sticker
    {54, "Side"},           // Buy/Sell
    {38, "OrderQty"},       // Quantity
    {40, "OrdType"},        // Order type: 1 = Market, 2 = Limit
    {44, "Price"},          // Price (only for limit)
    {59, "TimeInForce"}     // TIP: Day, GTC etc.
};

struct fix_data {
    std::vector<int> keys;
    std::unordered_map<int, std::string> fields;
};

std::pair<int, std::string> parse_field(std::string &string_field);

std::optional<fix_data> check_parsed_data(fix_data &data);

std::optional<fix_data> parse_fix_message(std::string fixMsg);

#endif
