#include <DAQInterface.h>

using namespace ToolFramework;

DAQInterface::DAQInterface(std::string configuration_file){

  vars.Initialise(configuration_file);
  if(!vars.Get("device_name",m_name)) m_name = "unnamed";
  vars.Set("service_name",m_name);

  
  
  m_context = new zmq::context_t(1);
  mp_SD = new ServiceDiscovery(true, false, 60000, "239.192.1.1", 5000, m_context, boost::uuids::random_generator()(), m_name, 5, 60);
  
  m_services= new Services();
  m_services->Init(vars, m_context, &sc_vars);
  
  
}
 
DAQInterface::~DAQInterface(){
  
  delete m_services;
  m_services=0;
  delete mp_SD;
  mp_SD=0;
  delete m_context;
  m_context=0;
  
}


// ===========================================================================
// Write Functions
// ---------------

bool DAQInterface::SendAlarm(const std::string& message, unsigned int level, const std::string& device, unsigned int timestamp, const unsigned int timeout){
  
  return m_services->SendAlarm(message, level, device, timestamp, timeout);
  
}

bool DAQInterface::SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& device, unsigned int timestamp, int* version, const unsigned int timeout){
  
  return m_services->SendCalibrationData(json_data, description, device, timestamp, version, timeout);
  
}

bool DAQInterface::SendDeviceConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device, unsigned int timestamp, int* version, const unsigned int timeout){
  
  return m_services->SendDeviceConfig(json_data, author, description, device, timestamp, version, timeout);
  
}

bool DAQInterface::SendRunConfig(const std::string& json_data, const std::string& name, const std::string& author, const std::string& description, unsigned int timestamp, int* version, const unsigned int timeout){
  
  return m_services->SendRunConfig(json_data, name, author, description, timestamp, version, timeout);
  
}

// ===========================================================================
// Read Functions
// --------------

bool DAQInterface::GetCalibrationData(std::string& json_data, int version, const std::string& device, const unsigned int timeout){
  
  return m_services->GetCalibrationData(json_data, version, device, timeout);
  
}

bool DAQInterface::GetDeviceConfig(std::string& json_data, int version, const std::string& device, const unsigned int timeout){
  
  return m_services->GetDeviceConfig(json_data, version, device, timeout);
  
}

bool DAQInterface::GetRunConfig(std::string& json_data, int config_id, const unsigned int timeout){
  
  return m_services->GetRunConfig(json_data, config_id, timeout);
  
}

bool DAQInterface::GetRunConfig(std::string& json_data, const std::string& name, int version, const unsigned int timeout){
  
  return m_services->GetRunConfig(json_data, name, version, timeout);
  
}

bool DAQInterface::GetDeviceConfigFromRunConfig(std::string& json_data, const int runconfig_id, const std::string& device, const unsigned int timeout){
  
  return m_services->GetRunDeviceConfig(json_data, runconfig_id, device, nullptr, timeout);
  
}

bool DAQInterface::GetDeviceConfigFromRunConfig(std::string& json_data, const std::string& runconfig_name, const int runconfig_version, const std::string& device, const unsigned int timeout){
  
  return m_services->GetRunDeviceConfig(json_data, runconfig_name, runconfig_version, device, nullptr, timeout);
  
}

bool DAQInterface::GetROOTplot(const std::string& plot_name, int& version, std::string& draw_options, std::string& json_data, std::string* timestamp, const unsigned int timeout){
  
  
  return m_services->GetROOTplot(plot_name, version, draw_options, json_data, timestamp, timeout);
  
}

bool DAQInterface::GetPlot(const std::string& name, Plot& plot, unsigned timeout){
  
  
  return m_services->GetPlot(name, plot, timeout);
  
}

bool DAQInterface::SQLQuery(const std::string& database, const std::string& query, std::vector<std::string>& responses, const unsigned int timeout){
  
  
  return m_services->SQLQuery(database, query, responses, timeout);
  
}

bool DAQInterface::SQLQuery(const std::string& database, const std::string& query, std::string& response, const unsigned int timeout){
  
  
  return m_services->SQLQuery(database, query, response, timeout);
}

bool DAQInterface::SQLQuery(const std::string& database, const std::string& query, const unsigned int timeout){
  
  return m_services->SQLQuery(database, query, timeout);
  
}

// ===========================================================================
// Multicast Senders
// -----------------

bool DAQInterface::SendLog(const std::string& message, unsigned int severity, const std::string& device, unsigned int timestamp){
    
  return m_services->SendLog(message, severity, device, timestamp);
  
}

bool DAQInterface::SendMonitoringData(const std::string& json_data, const std::string& device, unsigned int timestamp){
  
  
  return m_services->SendMonitoringData(json_data, device, timestamp);
  
}

// wrapper to send a root plot either to a temporary table or a persistent one
bool DAQInterface::SendROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, bool persistent, int* version, const unsigned int timestamp, const unsigned int timeout){
  if(!persistent) return SendTemporaryROOTplot(plot_name, draw_options, json_data, version, timestamp);
  return SendPersistentROOTplot(plot_name, draw_options, json_data, version, timestamp, timeout);
}

// send to persistent table over TCP
bool DAQInterface::SendPersistentROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version, const unsigned int timestamp, const unsigned int timeout){
  
  return m_services->SendPersistentROOTplot(plot_name, draw_options, json_data, version, timestamp, timeout);
  
}

// send to temporary table over multicast
bool DAQInterface::SendTemporaryROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version, const unsigned int timestamp){
  
  return m_services->SendTemporaryROOTplot(plot_name, draw_options, json_data, version, timestamp);
  
}

bool DAQInterface::SendPlot(Plot& plot, unsigned timeout){
  return m_services->SendPlot(plot, timeout);
}

// ===========================================================================
// Other functions
// ---------------

SlowControlCollection* DAQInterface::GetSlowControlCollection(){
  
  return &sc_vars;

}

SlowControlElement* DAQInterface::GetSlowControlVariable(std::string key){
  
  return sc_vars[key];
  
}

bool DAQInterface::AddSlowControlVariable(std::string name, SlowControlElementType type, std::function<std::string(const char*)> change_function, std::function<std::string(const char*)> read_function){
  
  return sc_vars.Add(name, type, change_function, read_function);
  
}

bool DAQInterface::RemoveSlowControlVariable(std::string name){
  
  return sc_vars.Remove(name);
  
}

void DAQInterface::ClearSlowControlVariables(){

  sc_vars.Clear();

}

bool DAQInterface::AlertSubscribe(std::string alert, std::function<void(const char*, const char*)> function){
  
  return sc_vars.AlertSubscribe(alert, function);
  
}
bool DAQInterface::AlertSend(std::string alert, std::string payload){
  
  return sc_vars.AlertSend(alert, payload);
  
}

std::string DAQInterface::PrintSlowControlVariables(){
  
  return sc_vars.Print();
  
}

std::string DAQInterface::GetDeviceName(){
  
  return m_name;
  
}
