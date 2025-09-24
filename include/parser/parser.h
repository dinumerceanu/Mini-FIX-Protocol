#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <optional>
#include <sstream>

constexpr char SOH = '\x01';

extern std::unordered_map<int, std::string> tag_names;

struct fix_data {
    std::vector<int> keys;
    std::unordered_map<int, std::string> fields;
};

std::pair<int, std::string> parse_field(std::string &string_field);

std::optional<fix_data> check_parsed_data(fix_data &data);

std::optional<fix_data> parse_fix_message(std::string fixMsg);

std::string fix_to_order_str(fix_data &data, std::string order_id);

#endif
