#ifndef DATA_MESSAGES_H
#define DATA_MESSAGES_H

#include <vector>
#include <iostream>
#include <zmq.hpp>
#include <chrono>
#include <BinaryStream.h>
#include <atomic>

using namespace ToolFramework;

void Decrement(void* data, void* hint);

class DataMessages: SerialisableObject{
  
public:
  
  DataMessages();
  
  bool Send(zmq::socket_t* sock, int flags = 0);  
  bool Print();
  bool Serialise(BinaryStream &bs); 
  
  
  std::vector<zmq::message_t> messages;
  uint16_t sent = 0;
  uint16_t error = 0;
  std::chrono::high_resolution_clock::time_point time;
  std::atomic<uint16_t> in_use;
  void* data = 0;
  size_t size = 0;
  
};


#endif

