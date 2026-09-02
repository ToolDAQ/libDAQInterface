#include <DataSender.h>

DataSender_args::DataSender_args():Thread_args(){}

DataSender_args::~DataSender_args(){}

void DeleteHeader(void* data){

  DAQHeader* tmp = reinterpret_cast<DAQHeader*>(data);
  delete tmp;

}

namespace {
  void FreeCachedPart(void* data){
    delete[] reinterpret_cast<char*>(data);
  }
}

bool CacheMessageToDisk(const std::string& cache_file, DataMessages* msg, uint64_t max_size_bytes){

  if(msg->messages.size() != 2) return false;   // we always expect [header, payload]

  uint32_t total_size = 0;
  for(auto& part : msg->messages) total_size += static_cast<uint32_t>(sizeof(uint32_t) + part.size());

  if(max_size_bytes > 0){
    std::error_code ec;
    uintmax_t current_size = std::filesystem::file_size(cache_file, ec);
    if(ec) current_size = 0;   // cache_file doesn't exist yet - treat as empty

    // +sizeof(total_size) for the footer this record would also append
    if(current_size + total_size + sizeof(total_size) > max_size_bytes) return false;
  }

  std::ofstream out(cache_file, std::ios::binary | std::ios::app);
  if(!out.is_open()) return false;

  for(auto& part : msg->messages){
    uint32_t part_size = static_cast<uint32_t>(part.size());
    out.write(reinterpret_cast<const char*>(&part_size), sizeof(part_size));
    out.write(reinterpret_cast<const char*>(part.data()), part_size);
  }

  out.write(reinterpret_cast<const char*>(&total_size), sizeof(total_size));

  return out.good();
}

DataMessages* RecoverMessageFromDisk(const std::string& cache_file){

  std::error_code ec;
  uintmax_t fsize = std::filesystem::file_size(cache_file, ec);
  if(ec || fsize < sizeof(uint32_t)) return nullptr;

  std::ifstream in(cache_file, std::ios::binary);
  if(!in.is_open()) return nullptr;

  uint32_t total_size = 0;
  in.seekg(static_cast<std::streamoff>(fsize) - static_cast<std::streamoff>(sizeof(total_size)));
  in.read(reinterpret_cast<char*>(&total_size), sizeof(total_size));

  if(total_size == 0 || sizeof(total_size) + total_size > fsize) return nullptr;   // corrupt/short record - bail rather than misread

  std::streamoff record_start = static_cast<std::streamoff>(fsize)
    - static_cast<std::streamoff>(sizeof(total_size)) - static_cast<std::streamoff>(total_size);
  in.seekg(record_start);

  DataMessages* msg = new DataMessages();

  for(int part = 0; part < 2; part++){
    uint32_t part_size = 0;
    in.read(reinterpret_cast<char*>(&part_size), sizeof(part_size));
    char* buf = new char[part_size];
    in.read(buf, part_size);
    msg->messages.emplace_back(buf, part_size, NullFree, nullptr);
    msg->delete_functions.emplace_back(FreeCachedPart);
  }

  in.close();

  std::filesystem::resize_file(cache_file, record_start);   // drop the record we just read off the tail

  return msg;
}

DataSender::DataSender(DAQInterface* interface, std::string config_file){

  args.daq_interface = interface;
  args.in_buffer = &in_buffer;

  Store config;
  config.Initialise(config_file);

  if(!LoadConfig(config)){
    std::cerr<<"Data socket missing configuration values"<<std::endl;
    args.daq_interface->SendLog("Data socket missing configuration values",LogLevel::Error);
  }

  args.sock = new zmq::socket_t(*(interface->GetContext()), ZMQ_DEALER);

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
    //args.sock->setsockopt(ZMQ_ROUTER_MANDATORY,router_mandatory);
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

  last = std::chrono::steady_clock::now();
  m_utils.CreateThread("SocketManager", &Thread, &args);

}

DataSender::~DataSender(){

  m_utils.KillThread(&args);

  delete args.sock;
  args.sock = 0;

}

void DataSender::Thread(Thread_args* arg){

  DataSender_args* args=reinterpret_cast<DataSender_args*>(arg);

  zmq::message_t msg;
  while(args->sock->recv(&msg, ZMQ_NOBLOCK)){

    if(msg.size() == 4){
      args->message_it = args->sent.find(*(reinterpret_cast<uint32_t*>(msg.data())));
      if(args->message_it != args->sent.end()){
        delete args->message_it->second;
        args->sent.erase(args->message_it);
        args->num_data_akn++;
      }
      continue;
    }
    std::cerr<<"received incorrect sized data aknowledgement "<<std::endl;
    args->daq_interface->SendLog("Received incorrect sized data aknowledgement",LogLevel::Error);

  }


  ///////////////////////////////////////////////////////////////


  //////////////////////// finding old data that needs resending /////////////////////
  for(args->message_it=args->sent.begin(); args->message_it!= args->sent.end();){

    args->time_span = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - args->message_it->second->time);
    if(args->time_span.count() > args->resend_period_ms){
      if(args->message_it->second->sent + args->message_it->second->error > args->retry_limit){
	//std::cout<<"in_use="<<args->message_it->second->in_use<<std::endl;
	if(args->message_it->second->in_use==0){

	  if(!args->cache_file.empty() && CacheMessageToDisk(args->cache_file, args->message_it->second, args->cache_max_size_bytes)){
	    args->num_data_cached++;
	  } else {
	    if(!args->cache_file.empty()){
	      args->daq_interface->SendLog("Failed to cache data to disk after max retries, data lost",LogLevel::Error);
	      std::cerr<<"Failed to cache data to disk after max retries, data lost"<<std::endl;
	    } else {
	      args->daq_interface->SendLog("Electronics Data being throw away as max retries reached",LogLevel::Warning);
	      std::cerr<<"Electronics Data being throw away as max retries reached"<<std::endl;
	    }
	    args->num_data_deleted++;
	  }

	  delete args->message_it->second;
	  args->message_it->second = 0;
	  args->message_it = args->sent.erase(args->message_it);
	}
	continue;

      }

      //std::cout<<"resent!!!!!"<<std::endl;
      args->to_send.push_back(args->message_it->second);
      args->message_it->second = 0;
      args->message_it = args->sent.erase(args->message_it);
    }
    else args->message_it++;
  }


  ///////////////////////////////////////////////////////////////

  /////////////// getting new data to send ///////////////////////////
  args->in_buffer->Swap(args->new_send);
  args->to_send.insert(args->to_send.end(),args->new_send.begin(),args->new_send.end());
  args->new_send.clear();
  ////////////////////////////////////////////////////////////////////

  // opportunistically recover disk-cached data, once nothing live is waiting
  if(args->to_send.size()==0 && !args->cache_file.empty()){
    for(uint32_t i=0; i<args->cache_recover_per_cycle; i++){
      DataMessages* recovered = RecoverMessageFromDisk(args->cache_file);
      if(!recovered) break;
      args->to_send.push_back(recovered);
      args->num_data_cached--;
    }
  }
  ////////////////////////////////////////////////////////////////////


  /// if no data to send and none to receive sleep /////////////
  if(args->to_send.size()==0){

    try{
      zmq::poll(&(args->in_items[0]), 1, args->poll_period_ms);
    }
    catch(zmq::error_t& e){
      if(zmq_errno()==EINTR) return;
      std::cerr<<"zmq_poll data receive poll caught "<<e.what()<<std::endl;
      args->daq_interface->SendLog("zmq::poll data receive poll caught "+std::string{e.what()},LogLevel::Error);
    }

    return;
  }


  ////////////////////////////////////////////////////////////

  /////////////////////////// sending data //////////////////////
  args->send_error_counter=0;
  while (!args->to_send.empty()){
    DataMessages* data_messages = args->to_send.front();

    if (args->send_error_counter < 100){
      if (data_messages->Send(args->sock, ZMQ_NOBLOCK)) data_messages->sent++;
      else{
	data_messages->error++;
	args->send_error_counter++;
      }
    }
    else{
      data_messages->error++;
    }

    data_messages->time = std::chrono::steady_clock::now();
    uint32_t msg_num = *reinterpret_cast<uint32_t*>(data_messages->messages[0].data());
    args->sent[msg_num] = data_messages;
    args->to_send.pop_front();
  }

  if (args->send_error_counter == 100) usleep(100);

}
  ////////////////////////////////////////////////////////



bool DataSender::Add(void* data, size_t size, uint32_t coarse_counter, std::function<void(void*)> del_func){

  DataMessages* message = new DataMessages(); //use pool;
  DAQHeader* header = new DAQHeader(coarse_counter, message_num++);
  header->SetCardID(card_id);
  header->SetType(card_type);
  header->SetNumWords(size/4);
  message->messages.emplace_back(header, header->Size(), NullFree, nullptr);
  message->delete_functions.emplace_back(DeleteHeader);
  message->messages.emplace_back(data, size, Decrement, message);
  message->delete_functions.emplace_back(del_func);

  return Add(message);
}

bool DataSender::Add(DataMessages* message){

  in_buffer.Add(message);
  args.num_data_messages++;
  return true;
}

std::string DataSender::Summary(){

  Store tmp;


  std::chrono::milliseconds time_span = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last);

  num_data_messages_rate = ((float)((args.num_data_messages - num_data_messages)*1000))/ (float)time_span.count();
  num_data_akn_rate = ((float)((args.num_data_akn - num_data_akn)*1000))/ (float)time_span.count();
num_data_deleted_rate = ((float)((args.num_data_deleted - num_data_deleted)*1000))/ (float)time_span.count();

  num_data_messages = args.num_data_messages;
  num_data_akn = args.num_data_akn;
  num_data_deleted = args.num_data_deleted;


  last = std::chrono::steady_clock::now();

  tmp.Set("num_data_messages_rate", num_data_messages_rate);
  tmp.Set("num_data_akn_rate", num_data_akn_rate);
  tmp.Set("num_data_deleted_rate", num_data_deleted_rate);

  tmp.Set("num_data_messages", num_data_messages);
  tmp.Set("num_data_akn", num_data_akn);
  tmp.Set("num_data_deleted", num_data_deleted);

  tmp.Set("sent_buffer_size", args.sent.size());
  tmp.Set("to_send_buffer_size", args.to_send.size());
  tmp.Set("num_data_cached", (uint64_t)args.num_data_cached);


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
  //ret = vars.Get("ZMQ_ROUTER_MANDATORY", router_mandatory) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE", tcp_keepalive) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE_IDLE", tcp_keepalive_idle_sec) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE_CNT", tcp_keepalive_count) && ret;
  ret = vars.Get("ZMQ_TCP_KEEPALIVE_INTVL", tcp_keepalive_interval_sec) && ret;
  ret = vars.Get("data_port", data_port) && ret;

  ret = vars.Get("retry_limit", args.retry_limit) && ret;
  ret = vars.Get("resend_period_ms", args.resend_period_ms) && ret;
  ret = vars.Get("poll_period_ms", args.poll_period_ms) && ret;

  ret = vars.Get("card_type", card_type) && ret;
  ret = vars.Get("card_id", card_id) && ret;

  vars.Get("cache_file", args.cache_file);
  vars.Get("cache_recover_per_cycle", args.cache_recover_per_cycle);
  vars.Get("cache_max_size_bytes", args.cache_max_size_bytes);

  args.num_data_messages = 0;
  args.num_data_akn = 0;
  args.num_data_deleted = 0;
  args.num_data_cached = 0;

  return ret;

}
