#ifndef DATA_MESSAGES_H
#define DATA_MESSAGES_H

#include <vector>
#include <iostream>
#include <zmq.hpp>
#include <chrono>
#include <BinaryStream.h>
#include <atomic>
#include <functional>

using namespace ToolFramework;

void NullFree(void*data, void* hint);
void Decrement(void* data, void* hint);

class DataMessages: SerialisableObject{
  
public:
  
  DataMessages();
  ~DataMessages();
  
  bool Send(zmq::socket_t* sock, int flags = 0);  
  bool Print();
  bool Serialise(BinaryStream &bs); 
  
  
  std::vector<zmq::message_t> messages;
  std::vector<std::function<void(void*)> > delete_functions;
  uint16_t sent = 0;
  uint16_t error = 0;
  std::chrono::steady_clock::time_point time;
  std::atomic<uint16_t> in_use;
  void* data = 0;
  size_t size = 0;
  
};


#endif

