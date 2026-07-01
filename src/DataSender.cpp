#include <DataSender.h>


DataSender::DataSender(DAQInterface* interface, Store& vars){
  
  args.daq_interface = interface;
  
  args.in_buffer = &in_buffer;

  if(!LoadConfig(vars)){
    std::cerr<<"Data socket missing configuration values"<<std::endl;
    args.daq_interface->SendLog("Data socket missing configuration values",LogLevel::Error);
  }

  args.sock = new zmq::socket_t(*(interface->context), ZMQ_DEALER);

  args.in_items[0].socket=*(args.sock);
  args.in_items[0].fd=0;
  args.in_items[0].events=ZMQ_POLLIN;
  args.in_items[0].revents=0;
  
  try {
    args.sock->setsockopt(ZMQ_SNDHWM, send_high_watermark);
    args.sock->setsockopt(ZMQ_RCVHWM, receive_high_watermark);
    args.sock->setsockopt(ZMQ_LINGER, linger_ms);
    args.sock->setsockopt(ZMQ_BACKLOG, backlog);
    args.sock->setsockopt(ZMQ_RCVTIMEO, receive_timeout_ms);
    args.sock->setsockopt(ZMQ_SNDTIMEO, send_timeout_ms);
    args.sock->setsockopt(ZMQ_IMMEDIATE, immediate);
    args.sock->setsockopt(ZMQ_ROUTER_MANDATORY,router_mandatory);
    args.sock->setsockopt(ZMQ_TCP_KEEPALIVE, tcp_keepalive);
    args.sock->setsockopt(ZMQ_TCP_KEEPALIVE_IDLE, tcp_keepalive_idle_sec);
    args.sock->setsockopt(ZMQ_TCP_KEEPALIVE_CNT, tcp_keepalive_count);
    args.sock->setsockopt(ZMQ_TCP_KEEPALIVE_INTVL, tcp_keepalive_interval_sec);
    args.sock->setsockopt(ZMQ_IDENTITY, interface->GetDeviceName().c_str(), interface->GetDeviceName().length());
    
    args.sock->bind("tcp://*:" + data_port);
    
  } catch(zmq::error_t& e){
    std::cerr<<"caught "<<e.what()<<" configuring data socket"<<std::endl;
    args.daq_interface->SendLog("error configuring data socket "+std::string{e.what()},LogLevel::Error);    
  }

  last = std::chrono::high_resolution_clock::now();
  m_utils.CreateThread("SocketManager", &Thread, &args);  

}

void DataSender::Thread(Thread_args* arg){
  
  DataSender_args* args=reinterpret_cast<DataSender_args*>(arg);
  
  zmq::message_t msg;
  while(args->sock->recv(&msg, ZMQ_NOBLOCK)){
    if(msg.size() == 4){
      args->sent.erase(*(reinterpret_cast<uint32_t*>(msg.data())));
      continue;
    }
    std::cerr<<"received incorrect sized data aknowledgement "<<std::endl;
    args->daq_interface->SendLog("Received incorrect sized data aknowledgement",LogLevel::Error);
    
  }
  
  
  ///////////////////////////////////////////////////////////////
  
  /////////////// getting new data to send ///////////////////////////
  args->in_buffer->Swap(args->to_send);
  ////////////////////////////////////////////////////////////////////
  
  //////////////////////// finding old data that needs resending /////////////////////
  for(std::map<uint32_t, DataMessages*>::iterator it=args->sent.begin(); it!= args->sent.end();){
    
    args->time_span = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - it->second->time);
    if(args->time_span.count() > args->resend_period_ms){
      if(it->second->sent + it->second->error > args->retry_limit){
	delete it->second;
	it->second = 0;
	it = args->sent.erase(it);
	args->daq_interface->SendLog("Electronics Data being throw away as max retries reached",LogLevel::Warning);
	args->num_data_deleted++;
	continue;
      }
      args->to_send.push_back(it->second);
      it->second = 0;
      it = args->sent.erase(it);
      args->num_data_akn++;
    }
    else it++;
  }
  
  
  ///////////////////////////////////////////////////////////////
  
  /// if no data to send and none to receive sleep /////////////
  if(args->to_send.size()==0){
    
    try{
      zmq::poll(&(args->in_items[0]), 1, args->poll_period_ms);
    }
    catch(zmq::error_t& e){
      std::cerr<<"zmq_poll data receive poll caught "<<e.what()<<std::endl;
      args->daq_interface->SendLog("zmq::poll data receive poll caught "+std::string{e.what()},LogLevel::Error);
    }
    
    return;
  }
  
  
  ////////////////////////////////////////////////////////////
  
  /////////////////////////// sending data //////////////////////  
  for(size_t i=0; i < args->to_send.size(); i++){
    
    if(args->to_send.at(i)->Send(args->sock,ZMQ_NOBLOCK)){ //no block or jsut use snd timeout?
      args->to_send.at(i)->sent++;	  
    }
    else{
      args->to_send.at(i)->error++;
    }
    
    args->to_send.at(i)->time=std::chrono::high_resolution_clock::now(); 
    args->sent[*(reinterpret_cast<uint32_t*>(args->to_send.at(i)->messages.at(0).data()))]=args->to_send.at(i);
    
  }
  
  args->to_send.clear();
  
  
  ////////////////////////////////////////////////////////
  
  
}


bool DataSender::Add(void* data, size_t size){

  DataMessages* message = new DataMessages(); //use pool;
  DAQHeader header;

  zmq::message_t msg_header(header.size())
    //blah

    zmq::message_t data( 
  
			
			return Add(message);
}

bool DataSender::Add(DataMessages* message){

  in_buffer.Add(message);
  args.num_data_messages++;
  return true;
}

std::string DataSender::Summary(){

  Store tmp;

  
  std::chrono::milliseconds time_span = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - last);
  
  num_data_messages_rate = ((args.num_data_messages - num_data_messages)*1000)/ (float)time_span.count(); 
  num_data_akn_rate = ((args.num_data_akn - num_data_akn)*1000)/ (float)time_span.count();  
  num_data_deleted_rate = ((args.num_data_deleted - num_data_deleted)*1000)/ (float)time_span.count(); 

  num_data_messages = args.num_data_messages;
  num_data_akn = args.num_data_akn;
  num_data_deleted = args.num_data_deleted;


  last = std::chrono::high_resolution_clock::now();
  
  tmp.Set("num_data_messages_rate", num_data_messages_rate);
  tmp.Set("num_data_akn_rate", num_data_akn_rate);
  tmp.Set("num_data_deleted_rate", num_data_deleted_rate);

  tmp.Set("num_data_messages", num_data_messages);
  tmp.Set("num_data_akn", num_data_akn);
  tmp.Set("num_data_deleted", num_data_deleted);
  

  std::string out;

  tmp>>out;

  return out;

}



bool DataSender::LoadConfig(std::string json){

  Store tmp;
  tmp.JsonParser(json);
  return LoadConfig(tmp);
}

  bool DataSender::LoadConfig(Store& vars){

  bool ret = true;
  ret = vars.Get("ZMQ_RCVHWM", receive_high_watermark) && ret;
  ret = vars.Get("ZMQ_SNDHWM", send_high_watermark) && ret;
  ret = vars.Get("ZMQ_LINGER", linger_ms) && ret;
  ret = vars.Get("ZMQ_BACKLOG", backlog) && ret;
  ret = vars.Get("ZMQ_RCVTIMEO", receive_timeout_ms) && ret;
  ret = vars.Get("ZMQ_SNDTIMEO", send_timeout_ms) && ret;
  ret = vars.Get("ZMQ_IMMEDIATE", immediate) && ret;
  ret = vars.Get("ZMQ_ROUTER_MANDATORY", router_mandatory) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE", tcp_keepalive) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE_IDLE", tcp_keepalive_idle_sec) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE_CNT", tcp_keepalive_count) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE_INTVL", tcp_keepalive_interval_sec) && ret;
  ret = vars.Get("data_port", data_port) && ret;  
  
  ret = vars.Get("retry_limit", args.retry_limit) && ret;  
  ret = vars.Get("resend_period_ms", args.resend_period_ms) && ret;  
  ret = vars.Get("poll_period_ms", args.poll_period_ms) && ret;

  args.num_data_messages = 0;
  args.num_data_akn = 0;
  args.num_data_deleted = 0;

  return ret;

}
