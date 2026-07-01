#ifndef DATA_MESSAGES_H
#define DATA_MESSAGES_H

#include <vector>
#include <iostream>
#include <zmq.hpp>
#include <chrono>
#include <BinaryStream.h>
//#include <SerialisableObject.h>

using namespace ToolFramework;

class DataMessages: SerialisableObject{
  
public:
  
  
  bool Send(zmq::socket_t* sock, int flags = 0){
    
    if(sock == 0) return false;
    for(size_t i=0 ; i<messages.size()-1; i++){
      sock->send(messages.at(i), ZMQ_SNDMORE);
    }
    return sock->send(messages.at(messages.size()-1), flags);
  }
  bool Print(){return true;}
  bool Serialise(BinaryStream &bs){
    
    
    size_t tmp=messages.size();
    bs << tmp;
    
    for(size_t i=0; i<tmp; i++){
      tmp=messages.at(i).size();
      bs<<tmp;
      bs.Bwrite(messages.at(i).data(), messages.at(i).size());
    }
    
    return true;
  }
  
  
  std::vector<zmq::message_t> messages;
  uint16_t sent = 0;
  uint16_t error = 0;
  std::chrono::high_resolution_clock::time_point time;
  
};


#endif

