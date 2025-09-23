#include "parser.h"

std::pair<int, std::string> parse_field(std::string &string_field) {
    size_t eq_pos = string_field.find('=');

    if (eq_pos == std::string::npos) {
        throw std::runtime_error("Invalid field: no '=' found");
    }

    try {
        int tag = std::stoi(string_field.substr(0, eq_pos));
        std::string value = string_field.substr(eq_pos + 1);
        return {tag, value};
    } catch (const std::invalid_argument&) {
        throw(std::runtime_error("Invalid tag (not a number): " + string_field));
    } catch (const std::out_of_range&) {
        throw(std::runtime_error("Tag out of range: " + string_field));
    }
}

std::optional<fix_data> check_parsed_data(fix_data &data) {
    // General required tags
    std::vector<int> requiredFields = {8, 9, 35, 34, 49, 56, 10};

    for (int tag : requiredFields) {
        if (data.fields.find(tag) == data.fields.end()) {
            std::cerr << "Missing required tag " << tag << std::endl;
            return std::nullopt;
        }
    }

    std::string msgType = data.fields[35];

    if (msgType == "A") {
        // Logon
        if (data.fields.find(108) == data.fields.end()) {
            return std::nullopt;
        } else {
            try {
                int hb = std::stoi(data.fields[108]);
                if (hb <= 0) {
                    std::cerr << "Heartbeat interval must be positive: " << hb << "\n";
                    return std::nullopt;
                }
            } catch (const std::exception &e) {
                std::cerr << "Invalid heartbeat interval: " << e.what() << "\n";
                return std::nullopt;
            }
        }
    } else if (msgType == "D") {
        // NewOrderSingle
        std::vector<int> requiredFields = {11, 55, 54, 38, 40};

        for (int tag : requiredFields) {
            if (data.fields.find(tag) == data.fields.end()) {
                std::cerr << "Missing required tag " << tag << " for NewOrderSingle\n";
                return std::nullopt;
            }
        }

        // Side
        try {
            int side = std::stoi(data.fields[54]);
            if (side != 1 && side != 2) {
                std::cerr << "Invalid Side value: " << side << "\n";
                return std::nullopt;
            }
        } catch (const std::exception &e) {
            std::cerr << "Invalid Side: " << e.what() << "\n";
            return std::nullopt;
        }

        // OrdType & Price if Limit
        try {
            int ordType = std::stoi(data.fields[40]);
            if (ordType != 1 && ordType != 2) {
                std::cerr << "Invalid OrdType: " << ordType << "\n";
                return std::nullopt;
            }

            if (ordType == 2) { // Limit order
                if (data.fields.find(44) == data.fields.end()) {
                    std::cerr << "Missing Price for Limit order\n";
                    return std::nullopt;
                }
                int price = std::stoi(data.fields[44]);
                if (price <= 0) {
                    std::cerr << "Price must be positive: " << price << "\n";
                    return std::nullopt;
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Invalid OrdType or Price: " << e.what() << "\n";
            return std::nullopt;
        }

        // OrderQty
        try {
            int qty = std::stoi(data.fields[38]);
            if (qty <= 0) {
                std::cerr << "OrderQty must be positive: " << qty << "\n";
                return std::nullopt;
            }
        } catch (const std::exception &e) {
            std::cerr << "Invalid OrderQty: " << e.what() << "\n";
            return std::nullopt;
        }
    } else {
        throw std::runtime_error("Not yet implemented!");
        return std::nullopt;
    }

    return data;
}

std::optional<fix_data> parse_fix_message(std::string fixMsg) {
    size_t start = 0;
    size_t end;
    fix_data data;

    try {
        while ((end = fixMsg.find(SOH, start)) != std::string::npos) {
            std::string string_field = fixMsg.substr(start, end - start);
            std::pair<int, std::string> parsed_field = parse_field(string_field);
            data.keys.push_back(parsed_field.first);
            data.fields.insert({parsed_field.first, parsed_field.second});
            start = end + 1;
        }
    } catch (const std::runtime_error &e) {
        std::cerr << "Parsing error: " << e.what() << std::endl;
        return std::nullopt;
    }

    return check_parsed_data(data);
}

int main() {
    std::string fixLogon;

    fixLogon += "8=FIX.4.4";       fixLogon += SOH; // BeginString
    fixLogon += "9=65";            fixLogon += SOH; // BodyLength (dummy)
    fixLogon += "35=A";            fixLogon += SOH; // MsgType = Logon
    fixLogon += "34=1";            fixLogon += SOH; // MsgSeqNum
    fixLogon += "49=CLIENT1";      fixLogon += SOH; // SenderCompID
    fixLogon += "56=SERVER";       fixLogon += SOH; // TargetCompID
    fixLogon += "108=30";          fixLogon += SOH; // HeartBtInt = 30 sec
    fixLogon += "10=123";          fixLogon += SOH; // CheckSum (dummy)

    std::string fixNewOrder;
    fixNewOrder += "8=FIX.4.4";        fixNewOrder += SOH; // BeginString
    fixNewOrder += "9=112";            fixNewOrder += SOH; // BodyLength (dummy)
    fixNewOrder += "35=D";              fixNewOrder += SOH; // MsgType = NewOrderSingle
    fixNewOrder += "34=2";              fixNewOrder += SOH; // MsgSeqNum
    fixNewOrder += "49=CLIENT1";        fixNewOrder += SOH; // SenderCompID
    fixNewOrder += "56=SERVER";         fixNewOrder += SOH; // TargetCompID
    fixNewOrder += "11=ORD12345";       fixNewOrder += SOH; // ClOrdID (Order ID)
    fixNewOrder += "21=1";              fixNewOrder += SOH; // HandlInst (1=automated)
    fixNewOrder += "55=NVDA";           fixNewOrder += SOH; // Symbol
    fixNewOrder += "54=1";              fixNewOrder += SOH; // Side (1=Buy)
    fixNewOrder += "38=100";            fixNewOrder += SOH; // OrderQty
    fixNewOrder += "40=2";              fixNewOrder += SOH; // OrdType (2=Limit)
    fixNewOrder += "44=500";            fixNewOrder += SOH; // Price (pentru limit)
    fixNewOrder += "59=0";              fixNewOrder += SOH; // TimeInForce (0=Day)
    fixNewOrder += "10=234";            fixNewOrder += SOH; // CheckSum (dummy)

    auto data_opt = parse_fix_message(fixNewOrder);

    if (!data_opt) {
        std::cout << "Invalid fix msg!\n";
        return 1;
    }

    fix_data data = *data_opt;
    for (auto key : data.keys) {
        std::cout << key << " " << tag_names[key] << " " << data.fields[key] << std::endl;
    }

    return 0;
}
