#include "mynet/task.h"
#include<tuple>
#include "mynet/connection.h"
using namespace std;
using namespace mynet;

enum class State{
    CONNECTED,DISCONNECTED
};

vector<char> encode(const string& msg){
    int len = msg.size();
    vector<char> buf(len + 4);
    auto n32 = htonl(len);
    memcpy(buf.data(),&n32,sizeof n32);
    for(int i = 0;i < len;i++){
        buf[i + 4] = msg[i];
    }
    return buf;
}

Task<tuple<State,std::string>> decode(const Connection::Ptr& conn) {
    auto buf = co_await conn->read(4);
    if(buf.size() == 0) co_return {State::DISCONNECTED,string()};

    auto n32 =  *reinterpret_cast<uint32_t*>(buf.data());
    auto msg_len = ntohl(n32);
    if(msg_len > 0xffff){
      log::Log(log::Error,"invalid length {}",msg_len);
      co_return {State::DISCONNECTED,string()};
    }
    buf = co_await conn->read(msg_len);
    co_return {State::CONNECTED,string(buf.begin(),buf.end())};
}