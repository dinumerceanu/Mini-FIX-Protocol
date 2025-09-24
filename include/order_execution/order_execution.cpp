#include "../headers.h"

OrderExecutionInfo newOrderExecInfo(std::string clOrdID, int orderSize) {
    return OrderExecutionInfo {
        clOrdID,
        0,
        orderSize,
        0,
        0,
        false,
        false,
    };
}

void parse_orderbook_msg(OrderExecutionInfo &info_in, std::string &msg) {
    std::istringstream iss(msg);
    std::string tmp;
    std::string type;

    iss >> tmp;
    iss >> tmp;
    iss >> type;

    char bracket, slash, bracketClose;
    int filled, total;
    iss >> bracket >> filled >> slash >> total >> bracketClose;

    iss >> tmp;
    iss >> info_in.lastPx;

    if (type == "filled") {
        info_in.lastShares = filled;
        info_in.cumQty = info_in.cumQty + filled;
        info_in.leavesQty = total - info_in.cumQty;
        info_in.isFilled = (info_in.leavesQty == 0);
    } else {
        info_in.orderUnfilled = true;
    }
}

void printOrdExecInfo(OrderExecutionInfo &info) {
    printf("%s cumQty: %d, leavesQty: %d, lastShares: %d, lastPrice: %f, filled: %d, order_unfilled: %d\n", info.clOrdID.c_str(), info.cumQty, info.leavesQty, info.lastShares, info.lastPx, info.isFilled, info.orderUnfilled);
}

std::string generateOrderID() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "ORD" + std::to_string(ms);
}

std::string getIdFromResp(std::string &s) {
    std::istringstream iss(s);
    std::string first, second;
    iss >> first >> second;
    return second;
}

std::string createExecutionReport(OrderExecutionInfo &info, Client &client) {
    std::string sender_id_field = "49=";
    sender_id_field += "127.0.0.1";
    sender_id_field += ":";
    sender_id_field += std::to_string(PORT);

    std::string target_id_field = "56=";
    target_id_field += client.ipStr;
    target_id_field += ":";
    target_id_field += std::to_string(client.port);
    
    std::ostringstream fix;

    fix << "8=FIX.4.4" << SOH;                       // BeginString
    fix << "9=0" << SOH;                             // BodyLength (dummy)
    fix << "35=8" << SOH;                            // MsgType = ExecutionReport
    fix << "34=..." << SOH;                          // MsgSeqNum
    fix << "49=" << sender_id_field << SOH;                   // SenderCompID
    fix << "56=" << target_id_field << SOH;                   // TargetCompID
    fix << "11=" << info.clOrdID << SOH;            // ClOrdID
    fix << "17=" << info.clOrdID << SOH;            // ExecID
    fix << "150=0" << SOH;                           // ExecType: 0 = New / 1 = Partial Fill / 2 = Fill (poți adapta)
    fix << "39=" << (info.orderUnfilled ? "0" : (info.isFilled ? "2" : "1")) << SOH; // OrdStatus: 1 = Partially filled, 2 = Filled
    fix << "38=" << (info.cumQty + info.leavesQty) << SOH; // OrderQty
    fix << "32=" << (info.orderUnfilled ? "0" : std::to_string(info.lastShares)) << SOH;         // LastShares
    fix << "31=" << (info.orderUnfilled ? "0" : std::to_string(info.lastPx)) << SOH;             // LastPx
    fix << "14=" << (info.orderUnfilled ? "0" : std::to_string(info.cumQty)) << SOH;             // CumQty
    fix << "151=" << (info.orderUnfilled ? "0" : std::to_string(info.leavesQty)) << SOH;         // LeavesQty
    fix << "10=000" << SOH;                         // CheckSum (dummy)

    return fix.str();
}
