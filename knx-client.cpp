#include "knx-client.h"
#include <iostream>
#include <sys/select.h>
#include <unistd.h>
#include "wbmqtt/utils.h"

TKNXClient::TKNXClient(){}
extern bool debug;

void TKNXClient::Send(std::string topic, std::string payload) {
    std::vector<std::string> tokens = StringSplit(topic, "/");

    if(tokens[1] != "devices" || tokens[2] != "knx") return; // or error, handle it later
    eibaddr_t addr = (std::stoi(tokens[3]) << 11) + (std::stoi(tokens[4])<< 8) + (std::stoi(tokens[5]));

    EIBConnection * conn = EIBSocketLocal("/tmp/eib");// make this global for client maybe
    if(debug) std::cout << (conn != NULL) << " addr " << addr << "\n";
    //uint8_t val[] = {'t','r','u','e', 0};
    int res = EIBOpenT_Group(conn, addr, 0);// we may want to open another type of connection some day
    if(debug) std::cout << res << "\n";

    res = EIBSendTPDU(conn,addr, payload.size(), (const uint8_t*)payload.c_str()); //after this mfussenegger/knx can read from localhost:6720 "\x00\x0f\x00\x27\x00\x00\x09\x05\x01\x05${payload}"
    //res = EIBSendAPDU(conn, payload.size(), (const uint8_t*)payload.c_str()); //after this mfussenegger/knx can read from localhost:6720 "\x00\x0f\x00\x27\x00\x00\x09\x05\x00${payload}"
    if(debug) std::cout << res << "\n";
    res = EIBComplete(conn);
    if(debug) std::cout << res << "\n";
    res = EIBClose(conn);
    if(debug) std::cout << res << "\n";
}

void TKNXClient::Loop(){
    // FIXME: select never returns, i.e. we newer get any data
    // via our Busmonitor connection
    EIBConnection * conn = EIBSocketLocal("/tmp/eib");
    //EIBConnection * conn = EIBSocketRemote("localhost", 6720);

    int res = EIBOpenVBusmonitor(conn);
    std::cout << "LOOP "<< res << "\n";
    int fd = EIB_Poll_FD(conn);
    std::cout << "FD" << fd << std::endl;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    int ret = select(fd+1, &set, NULL, NULL, NULL);
    std::cout << "select returned" << std::endl;
    if(ret == -1)
        std::cout << "Select error\n";
    if(ret == 0)
        std::cout << "should not happen\n";
    if(ret == 1)
        std::cout << "should be ok\n";

    ret = EIB_Poll_Complete(conn);
    std::cout <<"eib poll complete return "<< ret << "\n";
    uint8_t packet[2048];
    ret = EIBGetBusmonitorPacket(conn, 2048, packet);
    std::cout << "eib packet " << ret << "\n";
    for(int i = 0; i < ret; i++){
        std::cout << (int)packet[i]<< " ";
    }
    std::cout << "\n";
}