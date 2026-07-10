#include <DataMessages.h>

void NullFree(void*data, void* hint){;}

void Decrement(void* data, void* hint){

  DataMessages* dm= reinterpret_cast<DataMessages*>(hint);
  dm->in_use--;
}

DataMessages::DataMessages(){in_use=0;}

DataMessages::~DataMessages(){

  for(size_t i=0 ; i<messages.size(); i++){
    if(delete_functions.at(i)) delete_functions.at(i)(messages.at(i).data());
  }
  
}
  
bool DataMessages::Send(zmq::socket_t* sock, int flags){
  
  if(sock == 0) return false;
  // std::cout<<"in_use="<< in_use<<std::endl;
  in_use++;
  bool ret = true;
  for(size_t i=0 ; i<messages.size()-1; i++){
    data = messages.at(i).data();
    size = messages.at(i).size();
    ret = sock->send(messages.at(i), ZMQ_SNDMORE);
    messages.at(i).rebuild(data, size, NullFree, nullptr);  
    if(!ret){
      in_use--;
      //std::cout<<zmq_strerror(errno)<<std::endl;
      return ret;
    }
  }
  
  data = messages.at(messages.size()-1).data();
  size = messages.at(messages.size()-1).size();
  ret = ret & sock->send(messages.at(messages.size()-1), flags);
  //  if(ret==false)std::cout<<zmq_strerror(errno)<<std::endl;       
  messages.at(messages.size()-1).rebuild(data, size, Decrement, this);
  return ret;
}
  

bool DataMessages::Print(){return true;}

bool DataMessages::Serialise(BinaryStream &bs){ 
  
  size_t tmp=messages.size();
  bs << tmp;
  
  for(size_t i=0; i<tmp; i++){
    tmp=messages.at(i).size();
    bs<<tmp;
    bs.Bwrite(messages.at(i).data(), messages.at(i).size());
  }
  
  return true;
}


