#include <DAQInterface.h>
#include <cstdio>

using namespace ToolFramework;

DAQInterface::DAQInterface(std::string configuration_file, zmq::context_t* context){
  
  if(!vars.Initialise(configuration_file)){
    std::clog<<"Error invalid configuration file given to DAQ Interface: "<<configuration_file<<std::endl;
  }
  
  if(!vars.Get("device_name",m_name)) m_name = "unnamed";
  vars.Set("service_name",m_name);
  
  boost::uuids::uuid m_UUID;
  std::string s_uuid;
  
  if(vars.Get("UUID", s_uuid)){
    m_UUID = boost::uuids::string_generator{}(s_uuid);
    
  } else {
    
    std::string uuid_path;
    if(vars.Get("UUID_path", uuid_path)){
      
      FILE* fp = fopen(uuid_path.c_str(), "rb");
      
      if(fp){
        
        // UUID file exists; read it.
        if (fread(&m_UUID, sizeof(m_UUID), 1, fp) != 1){
          // Read failed; generate a new UUID.
          m_UUID = boost::uuids::random_generator()();
        }
        
        fclose(fp);
        
      } else {
        // UUID file doesn't exist; generate and save one.
        m_UUID = boost::uuids::random_generator()();
        
        fp = fopen(uuid_path.c_str(), "wb");
        if(fp){
          fwrite(&m_UUID, sizeof(m_UUID), 1, fp);
          fclose(fp);
        }
      }
      
    } else {
       // No UUID path specified; just generate one.
       m_UUID = boost::uuids::random_generator()();
    }
  }
  
  
  int pubsec = 5;
  int kicksec = 60;
  
  vars.Get("service_publish_sec", pubsec);
  vars.Get("service_kick_sec", kicksec);
  
  bool send = (pubsec>=0);
  bool receive = (kicksec>=0);
  int remote_port = 60000;
  
  vars.Get("sc_port", remote_port);
  
  std::vector<std::string> mc_addresses;
  std::vector<int> mc_ports;
  
  if(!vars.Get("service_discovery_address",mc_addresses)){
    std::string tmp_address="";
    if(vars.Get("service_discovery_address",tmp_address)) mc_addresses.emplace_back(tmp_address);
    else  mc_addresses.emplace_back("239.192.1.1");
  }
  
  if(!vars.Get("service_discovery_port",mc_ports)){
    int tmp_port=0;
    if(vars.Get("service_discovery_port",tmp_port)) mc_ports.emplace_back(tmp_port);
    else mc_ports.emplace_back(5000);
  }
  
  if(context==0){
    int iothreads = 1;
    vars.Get("zmq_io_threads",iothreads);
    
    m_context = new zmq::context_t(iothreads);
  } else {
     m_context = context;
  }
  
  mp_SD = new ServiceDiscovery(send, receive, remote_port, mc_addresses, mc_ports , m_context,m_UUID, m_name, pubsec,  kicksec);
  
  
  m_services= new Services();
  m_services->Init(vars, m_context, &sc_vars, true);
  
}

DAQInterface::~DAQInterface(){
  
  delete m_services;
  m_services=0;
  delete mp_SD;
  mp_SD=0;
  delete m_context;
  m_context=0;
  
}

void DAQInterface::SetVerbose(bool in){
  m_services->SetVerbose(in);
  return;
}

bool DAQInterface::Ready(int timeout_ms){
  return m_services->Ready(timeout_ms);
}

// ===========================================================================
// Write Functions
// ---------------

bool DAQInterface::SendAlarm(const std::string& message, bool critical, const std::string& device, const uint64_t timestamp, const unsigned int timeout){
  
  return m_services->SendAlarm(message, critical, device, timestamp, timeout);
  
}

bool DAQInterface::SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& device, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  return m_services->SendCalibrationData(json_data, description, device, timestamp, version, timeout);
  
}

bool DAQInterface::SendDeviceConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device, const uint64_t timestamp, int* version, const unsigned int timeout){
  
  return m_services->SendDeviceConfig(json_data, author, description, device, timestamp, version, timeout);
  
}

// ===========================================================================
// Read Functions
// --------------

bool DAQInterface::GetCalibrationData(std::string& json_data, int& version, const std::string& device, const unsigned int timeout){
  
  return m_services->GetCalibrationData(json_data, version, device, timeout);
  
}

bool DAQInterface::GetCalibrationData(std::string& json_data, int&& version, const std::string& device, const unsigned int timeout){
  
  return m_services->GetCalibrationData(json_data, version, device, timeout);
  
}

bool DAQInterface::GetDeviceConfig(std::string& json_data, int version, const std::string& device, const unsigned int timeout){
  
  return m_services->GetDeviceConfig(json_data, version, device, timeout);
  
}

bool DAQInterface::GetRunConfig(std::string& json_data, const int base_config_id, const int runmode_config_id, const unsigned int timeout){
  
  return m_services->GetRunConfig(json_data, base_config_id, runmode_config_id, timeout);
  
}

bool DAQInterface::GetRunModeConfig(std::string& json_data, const std::string& name, int version, const unsigned int timeout){
  
  return m_services->GetRunModeConfig(json_data, name, version, timeout);
  
}

bool DAQInterface::GetDeviceConfigFromRunConfig(std::string& json_data, const int base_config_id, const int runmode_config_id, const std::string& device, const unsigned int timeout){
  
  return m_services->GetRunDeviceConfig(json_data, base_config_id, runmode_config_id, device, nullptr, timeout);
  
}

/*
bool DAQInterface::GetDeviceConfigFromRunConfig(std::string& json_data, const std::string& runconfig_name, const int runconfig_version, const std::string& device, const unsigned int timeout){
  
  return m_services->GetRunDeviceConfig(json_data, runconfig_name, runconfig_version, device, nullptr, timeout);
  
}
*/

bool DAQInterface::GetROOTplot(const std::string& plot_name, std::string& draw_options, std::string& json_data, int& version, const unsigned int timeout){
  
  return m_services->GetROOTplot(plot_name, draw_options, json_data, version, timeout);
  
}

bool DAQInterface::GetROOTplot(const std::string& plot_name, std::string& draw_options, std::string& json_data, int&& version, const unsigned int timeout){
  
  return m_services->GetROOTplot(plot_name, draw_options, json_data, version, timeout);
  
}

bool DAQInterface::GetPlotlyPlot(const std::string& name, std::string& trace, std::string& layout, int& version, unsigned int timeout) {
  
  return m_services->GetPlotlyPlot(name, trace, layout, version, timeout);
  
}

zmq::context_t* DAQInterface::GetContext(){ return m_context;}

bool DAQInterface::GetPlotlyPlot(const std::string& name, std::string& trace, std::string& layout, int&& version, unsigned int timeout) {
  
  return m_services->GetPlotlyPlot(name, trace, layout, version, timeout);
  
}

bool DAQInterface::SQLQuery(const std::string& query, std::vector<std::string>& responses, const unsigned int timeout){
  
  return m_services->SQLQuery(query, responses, timeout);
  
}

bool DAQInterface::SQLQuery(const std::string& query, std::string& response, const unsigned int timeout){
  
  return m_services->SQLQuery(query, response, timeout);
  
}

bool DAQInterface::SQLQuery(const std::string& query, const unsigned int timeout){
  
  return m_services->SQLQuery(query, timeout);
  
}

// ===========================================================================
// Multicast Senders
// -----------------

bool DAQInterface::SendLog(const std::string& message, LogLevel severity, const std::string& device, const uint64_t timestamp){
    
  return m_services->SendLog(message, severity, device, timestamp);
  
}

bool DAQInterface::SendMonitoringData(const std::string& json_data, const std::string& subject, const std::string& device, const uint64_t timestamp){
  
  return m_services->SendMonitoringData(json_data, subject, device, timestamp);
  
}

bool DAQInterface::SendROOTplot(const std::string& plot_name, const std::string& draw_options, const std::string& json_data, int* version, const uint64_t timestamp, const unsigned int lifetime, const unsigned int timeout){
  
  return m_services->SendROOTplot(plot_name, draw_options, json_data, version, timestamp, lifetime, timeout);
  
}

bool DAQInterface::SendPlotlyPlot(const std::string& name, const std::string& trace, const std::string& layout, int* version, const uint64_t timestamp, const unsigned int lifetime, unsigned int timeout) {
  
  return m_services->SendPlotlyPlot(name, trace, layout, version, timestamp, lifetime, timeout);
  
}

bool DAQInterface::SendPlotlyPlot(const std::string& name, const std::vector<std::string>& traces, const std::string& layout, int* version, const uint64_t timestamp, const unsigned int lifetime, unsigned int timeout) {
  
  return m_services->SendPlotlyPlot(name, traces, layout, version, timestamp, lifetime, timeout);
  
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

// SCFunction defined in SlowControlElement.h: std::function<std::string(const char*)>
bool DAQInterface::AddSlowControlVariable(std::string name, SlowControlElementType type, SCFunction change_function, SCFunction read_function, bool testing_lock, bool hidded){
  
  return sc_vars.Add(name, type, change_function, read_function, testing_lock, hidded);
  
}

bool DAQInterface::RemoveSlowControlVariable(std::string name){
  
  return sc_vars.Remove(name);
  
}

void DAQInterface::ClearSlowControlVariables(){
  
  sc_vars.Clear();
  
}

// AlertFunction defined in SlowControlCollection.h: std::function<bool(const char*, const char*)>
bool DAQInterface::AlertSubscribe(std::string alert, AlertFunction function){
  
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

std::string DAQInterface::GetLocalConfig(){
  
  return m_services->GetLocalConfig();
  
}

bool DAQInterface::SetLocalConfig(std::string json){
  
  return m_services->SetLocalConfig(json);
  
}

void DAQInterface::SetActive(bool active){
  return sc_vars.SetActive(active);
}

void DAQInterface::SetError(bool error){
  return sc_vars.SetError(error);
}

void DAQInterface::SetWarning(bool warn){
  return sc_vars.SetWarning(warn);
}

void DAQInterface::ClearState(){
  return sc_vars.ClearState();
}

bool DAQInterface::SetChangeConfigFunc(std::function<bool(std::string)> func){
  
  // we'll make three things that call this function:
  // 1. subscribe to alerts.
  // 2. a BUTTON slow control.
  // 3. a COMMAND slow control.
  // However, the signatures here are different:
  // Alert callbacks receive an alert name and payload, and return a bool.
  // Slow controls receive a control name, and return a string.
  // COMMANDs have an associated string variable in the SlowControlCollection, but buttons don't.
  // The idea here is the Alert and BUTTON will retrieve their configuration from the LocalConfig member of
  // the SlowControlCollection, while the COMMAND will get its configuration from the slow control variable.
  // To align these we accept something whihc is none of these, and pass it whatever's appropriate.
  bool allgood=true;
  
  // 1.
  allgood = AlertSubscribe("ChangeConfig", [this, func](const char*, const char*) -> bool{
    return func(m_services->GetLocalConfig());
  });
  
  // 2.
  allgood = allgood &&
  sc_vars.Add("ChangeConfig",
              BUTTON,
              [this, func](const char*) -> std::string {
                sc_vars["Config"]->SetValue((int)ConfigState::ChangeStart);
                bool ok = func(m_services->GetLocalConfig());
                if(!ok) sc_vars["State"]->SetValue((int)State::Warning);
                int new_state = ok ? (int)ConfigState::ChangeEnd : (int)ConfigState::ChangeFail;
                sc_vars["Config"]->SetValue(new_state);
                sc_vars["NewConfig"]->SetValue(0);
                // FIXME dcsdue to a present bug in the webserver, if we leave a JSON value
                // in a slow control, it will break the display of slow controls. :(
                sc_vars["Config"]->SetValue<std::string>("");
                return (ok ? "OK" : "Error");
              },
              0,
              false); // this version will not be locked during non-testing runs,
                      // since it only allows loading configurations in line with the current run type.
  
  // 3.
  allgood = allgood &&
  sc_vars.Add("ChangeToConfig",
              COMMAND,
              [this, func](const char* name) -> std::string {
                sc_vars["Config"]->SetValue((int)ConfigState::ChangeStart);
                bool ok = func(sc_vars[name]->GetValue<std::string>());
                int new_state = ok ? (int)ConfigState::ChangeEnd : (int)ConfigState::ChangeFail;
                sc_vars["Config"]->SetValue(new_state);
                if(!ok) sc_vars["State"]->SetValue((int)State::Warning);
                // FIXME due to a present bug in the webserver, if we leave a JSON value
                // in a slow control, it will break the display of slow controls. :(
                sc_vars["Config"]->SetValue<std::string>("");
                return (ok ? "OK" : "Error");
              },
              0,
              true); // this version will be locked during non-testing runs,
                     // since it allows loading arbitrary configurations
  
  return allgood;
  
}

bool DAQInterface::SetRunStopFunc(std::function<bool()> func){
  
  // we'll make three things that call this function:
  // 1. subscribe to alerts.
  // 2. a BUTTON slow control.
  // 3. a COMMAND slow control.
  // However, the signatures here are different:
  // Alert callbacks receive an alert name and payload, and return a bool.
  // Slow controls receive a control name, and return a string.
  // COMMANDs have an associated string variable in the SlowControlCollection, but buttons don't.
  // The idea here is the Alert and BUTTON will retrieve their configuration from the LocalConfig member of
  // the SlowControlCollection, while the COMMAND will get its configuration from the slow control variable.
  // To align these we accept something whihc is none of these, and pass it whatever's appropriate.
  bool allgood=true;
  
  // 1.
  allgood = AlertSubscribe("RunStop", [this, func](const char*, const char*) -> bool{
    return func();
  });
  
  // 2.
  allgood = allgood &&
    sc_vars.Add("RunStop",  //here ben
              BUTTON,
              [this, func](const char*) -> std::string {
                bool ok = func();
                if(!ok) SetWarning(true);
		sc_vars["Config"]->SetValue((int)ConfigState::Unconfigured);
                return (ok ? "OK" : "Error");
              },
              0,
              false); // this version will not be locked during non-testing runs,
                      // since it only allows loading configurations in line with the current run type.
  
  // 3.
  
  return allgood;
  
}

bool DAQInterface::SetExportConfigFunc(std::function<bool(std::string&)> func){
  
  // we'll make three things that call this function:
  // 1. subscribe to alerts.
  // 2. a BUTTON slow control.
  // 3. a COMMAND slow control.
  // However, the signatures here are different:
  // Alert callbacks receive an alert name and payload, and return a bool.
  // Slow controls receive a control name, and return a string.
  // COMMANDs have an associated string variable in the SlowControlCollection, but buttons don't.
  // The idea here is the Alert and BUTTON will retrieve their configuration from the LocalConfig member of
  // the SlowControlCollection, while the COMMAND will get its configuration from the slow control variable.
  // To align these we accept something whihc is none of these, and pass it whatever's appropriate.
  bool allgood=true;
  
  // 1.
  allgood = AlertSubscribe("ExportConfig", [this, func](const char*, const char*) -> bool{
    return func(tmp_config);
  });
  
  // 2.
  allgood = allgood &&
  sc_vars.Add("ExportConfig",
              BUTTON,
              [this, func](const char*) -> std::string {

		bool ok = func(tmp_config);
                if(!ok) SetWarning(true);
              	return tmp_config;
                },
              0,
              false); // this version will not be locked during non-testing runs,
                      // since it only allows loading configurations in line with the current run type.
  
  
  return allgood;
  
}
