#include "../headers.h"

ssize_t send_server_logon(Client &client) {
    std::string sender_id_field = "49=";
    sender_id_field += "127.0.0.1";
    sender_id_field += ":";
    sender_id_field += std::to_string(PORT);

    std::string target_id_field = "56=";
    target_id_field += client.ipStr;
    target_id_field += ":";
    target_id_field += std::to_string(client.port);

    std::string fixLogon;

    fixLogon += "8=FIX.4.4";            fixLogon += SOH; // BeginString
    fixLogon += "9=65";                 fixLogon += SOH; // BodyLength (dummy)
    fixLogon += "35=A";                 fixLogon += SOH; // MsgType = Logon
    fixLogon += "34=1";                 fixLogon += SOH; // MsgSeqNum
    fixLogon += sender_id_field;        fixLogon += SOH; // SenderCompID
    fixLogon += target_id_field;        fixLogon += SOH; // TargetCompID
    fixLogon += "108=30";               fixLogon += SOH; // HeartBtInt = 30 sec
    fixLogon += "10=123";               fixLogon += SOH; // CheckSum (dummy)

    return send(client.fd, fixLogon.c_str(), fixLogon.size(), 0);
}

ssize_t send_server_heartbeat(Client &client) {
    std::string sender_id_field = "49=";
    sender_id_field += "127.0.0.1";
    sender_id_field += ":";
    sender_id_field += std::to_string(PORT);

    std::string target_id_field = "56=";
    target_id_field += client.ipStr;
    target_id_field += ":";
    target_id_field += std::to_string(client.port);

    std::string fixHeartbeat;
    fixHeartbeat += "8=FIX.4.4"; fixHeartbeat += '\x01';
    fixHeartbeat += "9=65";                 fixHeartbeat += SOH; // BodyLength (dummy)
    fixHeartbeat += "35=0";      fixHeartbeat += '\x01'; // MsgType=Heartbeat
    fixHeartbeat += "34=...";    fixHeartbeat += '\x01'; // MsgSeqNum
    fixHeartbeat += sender_id_field; fixHeartbeat += '\x01';
    fixHeartbeat += target_id_field; fixHeartbeat += '\x01';
    fixHeartbeat += "10=123";    fixHeartbeat += '\x01'; // Checksum dummy

    return send(client.fd, fixHeartbeat.c_str(), fixHeartbeat.size(), 0);
}

ssize_t send_server_test_req(Client &client) {
    std::string sender_id_field = "49=";
    sender_id_field += "127.0.0.1";
    sender_id_field += ":";
    sender_id_field += std::to_string(PORT);

    std::string target_id_field = "56=";
    target_id_field += client.ipStr;
    target_id_field += ":";
    target_id_field += std::to_string(client.port);

    std::string fixTestRequest;
    fixTestRequest += "8=FIX.4.4";  fixTestRequest += '\x01';   // BeginString
    fixTestRequest += "9=65";       fixTestRequest += '\x01';   // BodyLength (dummy)
    fixTestRequest += "35=1";       fixTestRequest += '\x01';   // MsgType = TestRequest
    fixTestRequest += "34=...";     fixTestRequest += '\x01';   // MsgSeqNum
    fixTestRequest += sender_id_field; fixTestRequest += '\x01'; // SenderCompID
    fixTestRequest += target_id_field; fixTestRequest += '\x01'; // TargetCompID
    fixTestRequest += "112=TEST1";  fixTestRequest += '\x01';   // TestReqID
    fixTestRequest += "10=123";     fixTestRequest += '\x01';   // CheckSum (dummy)

    return send(client.fd, fixTestRequest.c_str(), fixTestRequest.size(), 0);
}

void send_client_logon(int clientSocket, sockaddr_in &clientAddress) {
    socklen_t clientAddressSize = sizeof(clientAddress);
    getsockname(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
    int port = ntohs(clientAddress.sin_port);

    std::string sender_id_field = "49=";
    sender_id_field += ip;
    sender_id_field += ":";
    sender_id_field += std::to_string(port);

    std::string fixLogon;

    fixLogon += "8=FIX.4.4";            fixLogon += SOH; // BeginString
    fixLogon += "9=65";                 fixLogon += SOH; // BodyLength (dummy)
    fixLogon += "35=A";                 fixLogon += SOH; // MsgType = Logon
    fixLogon += "34=1";                 fixLogon += SOH; // MsgSeqNum
    fixLogon += sender_id_field;        fixLogon += SOH; // SenderCompID
    fixLogon += "56=SERVER";            fixLogon += SOH; // TargetCompID
    fixLogon += "108=30";               fixLogon += SOH; // HeartBtInt = 30 sec
    fixLogon += "10=123";               fixLogon += SOH; // CheckSum (dummy)

    send(clientSocket, fixLogon.c_str(), fixLogon.size(), 0);
}

ssize_t send_client_heartbeat(int clientSocket, sockaddr_in &clientAddress) {
    socklen_t clientAddressSize = sizeof(clientAddress);
    getsockname(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
    int port = ntohs(clientAddress.sin_port);

    std::string sender_id_field = "49=";
    sender_id_field += ip;
    sender_id_field += ":";
    sender_id_field += std::to_string(port);

    std::string fixHeartbeat;
    fixHeartbeat += "8=FIX.4.4"; fixHeartbeat += '\x01';
    fixHeartbeat += "9=65";                 fixHeartbeat += SOH; // BodyLength (dummy)
    fixHeartbeat += "35=0";      fixHeartbeat += '\x01'; // MsgType=Heartbeat
    fixHeartbeat += "34=...";    fixHeartbeat += '\x01'; // MsgSeqNum
    fixHeartbeat += sender_id_field; fixHeartbeat += '\x01';
    fixHeartbeat += "56=SERVER"; fixHeartbeat += '\x01';
    fixHeartbeat += "10=123";    fixHeartbeat += '\x01'; // Checksum dummy

    return send(clientSocket, fixHeartbeat.c_str(), fixHeartbeat.size(), 0);
}

ssize_t send_client_test_req(int clientSocket, sockaddr_in &clientAddress) {
    socklen_t clientAddressSize = sizeof(clientAddress);
    getsockname(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddress.sin_addr, ip, sizeof(ip));
    int port = ntohs(clientAddress.sin_port);

    std::string sender_id_field = "49=";
    sender_id_field += ip;
    sender_id_field += ":";
    sender_id_field += std::to_string(port);

    std::string fixTestRequest;
    fixTestRequest += "8=FIX.4.4";  fixTestRequest += '\x01';   // BeginString
    fixTestRequest += "9=65";       fixTestRequest += '\x01';   // BodyLength (dummy)
    fixTestRequest += "35=1";       fixTestRequest += '\x01';   // MsgType = TestRequest
    fixTestRequest += "34=...";     fixTestRequest += '\x01';   // MsgSeqNum (incrementat de tine)
    fixTestRequest += sender_id_field; fixTestRequest += '\x01'; // SenderCompID
    fixTestRequest += "56=SERVER"; fixTestRequest += '\x01'; // TargetCompID
    fixTestRequest += "112=TEST1";  fixTestRequest += '\x01';   // TestReqID (orice string unic)
    fixTestRequest += "10=123";     fixTestRequest += '\x01';   // CheckSum (dummy)

    return send(clientSocket, fixTestRequest.c_str(), fixTestRequest.size(), 0);
}
