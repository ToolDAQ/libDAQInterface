#ifndef DAQ_INTERFACE_H
#define DAQ_INTERFACE_H

#include <ServiceDiscovery.h>
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <SlowControlCollection.h>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/progress.hpp>
#include <ServicesBackend.h>

namespace ToolFramework {

class DAQInterface{

 private:

  zmq::context_t* m_context=nullptr;
  bool free_context=false;
  ServiceDiscovery* mp_SD;
  ServicesBackend m_scclient;
  std::string m_dbname;
  std::string m_name;

 public:

  DAQInterface(zmq::context_t* context_in=nullptr);
  ~DAQInterface();
  bool Init(std::string name, std::string client_configfile, std::string db_name);
  
  
  bool SQLQuery(const std::string& database, const std::string& query, std::vector<std::string>* responses=nullptr, const unsigned int timeout=300);
  bool SQLQuery(const std::string& database, const std::string& query, std::string* response=nullptr, const unsigned int timeout=300);
  
  bool SendLog(const std::string& message, unsigned int severity=2, const std::string& device="", const unsigned int timestamp=0);
  bool SendAlarm(const std::string& message, unsigned int level=0, const std::string& device="", const unsigned int timestamp=0, const unsigned int timeout=300);
  bool SendMonitoringData(const std::string& json_data, const std::string& device="", unsigned int timestamp=0);
  bool SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& device="", unsigned int timestamp=0, int* version=nullptr, const unsigned int timeout=300);
  bool GetCalibrationData(std::string& json_data, int version=-1, const std::string& device="", const unsigned int timeout=300);
  bool SendConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device="", unsigned int timestamp=0, int* version=nullptr, const unsigned int timeout=300);
  bool GetConfig(std::string& json_data, int version=-1, const std::string& device="", const unsigned int timeout=300);
  bool SendROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, bool persistent=false, int* version=nullptr, const unsigned int timestamp=0, const unsigned int timeout=300);
  bool SendTemporaryROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version=nullptr, const unsigned int timestamp=0);
  bool SendPersistentROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version=nullptr, const unsigned int timestamp=0, const unsigned int timeout=300);
  bool GetROOTplot(const std::string& plot_name, int& version, std::string& draw_option, std::string& json_data, std::string* timestamp=nullptr, const unsigned int timeout=300);
  bool SendPlot();
  bool GetPlot();
  
  SlowControlCollection* GetSlowControlCollection();
  SlowControlElement* GetSlowControlVariable(std::string key);
  bool AddSlowControlVariable(std::string name, SlowControlElementType type, std::function<std::string(const char*)> function=nullptr);
  bool RemoveSlowControlVariable(std::string name);
  void ClearSlowControlVariables();

  bool AlertSubscribe(std::string alert, std::function<void(const char*, const char*)> function);
  bool AlertSend(std::string alert, std::string payload);
  
  std::string PrintSlowControlVariables();
  std::string GetDeviceName();

  template<typename T> T GetSlowControlValue(std::string name){
    return sc_vars[name]->GetValue<T>();
  }

  SlowControlCollection sc_vars;

private:

  bool SendCommand(const std::string& topic, const std::string& cmd_string, std::vector<std::string>* results=nullptr, std::string* err=nullptr,  const unsigned int timeout=300);
  bool SendCommand(const std::string& topic, const std::string& cmd_string, std::string* result=nullptr, std::string* err=nullptr, const unsigned int timeout=300);
  bool SendCommand(const std::string& cmd_string, std::string* err=nullptr);

  std::string escape_json(const std::string& s);

  
};

}

#endif
