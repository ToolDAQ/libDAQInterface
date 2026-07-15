#ifndef DATA_SENDER_H
#define DATA_SENDER_H

#include <DAQInterface.h>
#include <vector>
#include <deque>
#include <DataMessages.h>
#include <Utilities.h>
#include <mutex>
#include <chrono>
#include <atomic>
#include <Buffer.h>
#include <DAQHeader.h>
#include <functional>

using namespace ToolFramework;

void DeleteHeader(void* data);

struct DataSender_args: Thread_args{
  
  DataSender_args();
  ~DataSender_args();
  
  DAQInterface* daq_interface;
  Buffer<DataMessages*>* in_buffer;
  std::vector<DataMessages*> new_send; 
  std::deque<DataMessages*> to_send; 
  std::map<uint32_t, DataMessages*> sent;
  std::chrono::milliseconds time_span;
  std::map<uint32_t, DataMessages*>::iterator message_it;
 
  uint16_t retry_limit = 200;
  uint32_t resend_period_ms = 50;  
  uint32_t poll_period_ms = 100;
  uint16_t send_error_counter = 0;

  zmq::socket_t* sock;

  zmq::pollitem_t in_items[1];

  std::atomic<uint64_t> num_data_messages;
  std::atomic<uint64_t> num_data_akn;
  std::atomic<uint64_t> num_data_deleted;
 
};

class DataSender{
  
 public:
  
  DataSender(DAQInterface* interface, std::string config_file);

  bool LoadConfig(std::string json);
  bool LoadConfig(Store& vars);
  bool Add(void* data, size_t size, uint32_t coarse_counter, std::function<void(void*)> del_func);
  bool Add(DataMessages* message);
  
  std::string Summary();
  
  
  
 private:
  
  static void Thread(Thread_args* arg);

  DAQInterface* daq_interface;
  Utilities m_utils;  
  DataSender_args args;
  Buffer<DataMessages*> in_buffer;

  int32_t send_high_watermark = 1;
  int32_t receive_high_watermark = 20000;
  int32_t linger_ms = 100;
  int32_t backlog = 5000;
  int32_t receive_timeout_ms = 1000;
  int32_t send_timeout_ms = 200;
  int32_t immediate = 1;
  int32_t router_mandatory = 1;
  int32_t tcp_keepalive = 1;
  int32_t tcp_keepalive_idle_sec = 5;
  int32_t tcp_keepalive_count = 12;
  int32_t tcp_keepalive_interval_sec = 5;
 
  std::string data_port = "";

  uint64_t message_num = 0;
  
  uint64_t num_data_messages = 0;
  uint64_t num_data_akn = 0;
  uint64_t num_data_deleted = 0;
  float num_data_messages_rate = 0; 
  float num_data_akn_rate = 0; 
  float num_data_deleted_rate = 0;
  std::chrono::time_point<std::chrono::steady_clock> last;

  uint8_t card_type = 0;
  uint16_t card_id = 0;
  
};

#endif
