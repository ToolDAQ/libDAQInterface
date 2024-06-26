#include <DAQInterface.h>

using namespace ToolFramework;

DAQInterface::DAQInterface(){
  
  sc_vars.InitThreadedReceiver(&m_context, 88888, 100, false);
  
  m_scclient.SetUp(&m_context);
  
}
 
DAQInterface::~DAQInterface(){
  
  m_scclient.Finalise();
  sc_vars.Clear();
  delete mp_SD;
  mp_SD=0;
  
}

bool DAQInterface::Init(std::string name, std::string client_configfile, std::string db_name){
  
  m_name=name;
  m_dbname=db_name;
  
  mp_SD = new ServiceDiscovery(true, false, 88888, "239.192.1.1", 5000, &m_context, boost::uuids::random_generator()(), name, 5, 60);
   
  if(!m_scclient.Initialise(client_configfile)){
    std::cerr<<"error initialising slowcontrol client"<<std::endl;
    return false;
  }
   
  // TODO FIXME with better mechanism
  // after Initilising the slow control client needs ~15 seconds for the middleman to connect
  std::this_thread::sleep_for(std::chrono::seconds(15));
  // hopefully the middleman has found us by now
  
  return true;
}

// ===========================================================================
// Slow Control Base Functions
// ---------------------------

// Send a command over zmq socket and get multiple results (e.g. db query returning multiple rows)
bool DAQInterface::SendCommand(const std::string& topic, const std::string& cmd_string, std::vector<std::string>* results, std::string* err, const unsigned int timeout){
  //std::cout<<"DAQInterface sending "<<topic<<" command: '"<<cmd_string<<"'"<<std::endl;
  
  if(results) results->clear();
  
  // delivery within the timeout will be confirmed
  // (timeout does not necessarily mean failure to deliver)
  
  return m_scclient.SendCommand(topic, cmd_string, results, &timeout, err);
  
  // responses are returned as JSON maps, where each key represents a fieldname
  // and the corresponding value the field value. if multiple rows are returned,
  // this applies to each vector entry.
  
}

// Send a command over zmq socket and get a single result (e.g. db command returning a single row)
bool DAQInterface::SendCommand(const std::string& topic, const std::string& cmd_string, std::string* result, std::string* err, const unsigned int timeout){
  //std::cout<<"DAQInterface sending "<<topic<<" command: '"<<cmd_string<<"'"<<std::endl;
  
  if(result) *result="";
  
  return m_scclient.SendCommand(topic, cmd_string, result, &timeout, err);
  
}

// Send a multicast command and get no result (uses UDP, not guaranteed delivery)
bool DAQInterface::SendCommand(const std::string& cmd_string, std::string* err){
  //std::cout<<"DAQInterface sending multicast command '"<<cmd_string<<"'"<<std::endl;
  
  return m_scclient.SendMulticast(cmd_string, err);
  
}

// ===========================================================================
// Write Functions
// ---------------

bool DAQInterface::SendAlarm(const std::string& message, unsigned int level, const std::string& device, unsigned int timestamp, const unsigned int timeout){
  
  const std::string& name = (device=="") ? m_name : device;
  
  // we only handle 2 levels of alarm: critical (level 0) and normal (level!=0).
  if(level>0) level=1;
  
  std::string cmd_string = "{\"time\":"+std::to_string(timestamp)
                         + ",\"device\":\""+escape_json(name)+"\""
                         + ",\"level\":"+std::to_string(level)
                         + ",\"message\":\"" + escape_json(message) + "\"}";
  
  std::string err="";
  
  // send the alarm on the pub socket
  bool ok = SendCommand("W_ALARM", cmd_string, (std::vector<std::string>*)nullptr, &err, timeout);
  if(!ok){
    std::cerr<<"SendAlarm error: "<<err<<std::endl;
  }
  
  // also record it to the logging socket
  cmd_string = "{\"time\":"+std::to_string(timestamp)
             + ",\"device\":\""+escape_json(name)+"\""
             + ",\"severity\":0"
             + ",\"message\":\"" + escape_json(message) + "\"}";
  
  ok = SendCommand(cmd_string, &err);
  
  if(!ok){
    std::cerr<<"SendAlarm (log) error: "<<err<<std::endl;
  }
  
  return ok;
  
}

bool DAQInterface::SendCalibrationData(const std::string& json_data, const std::string& description, const std::string& device, unsigned int timestamp, int* version, const unsigned int timeout){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+escape_json(name)+"\""
                         + ", \"description\":\""+escape_json(description)+"\""
                         + ", \"data\":\""+escape_json(json_data)+"\" }";
  
  std::string err="";
  std::string response="";
  
  if(!SendCommand("W_CALIBRATION", cmd_string, &response, &err, timeout)){
    std::cerr<<"SendCalibrationData error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created config entry
  // e.g. '{"version":3}'. check this is what we got, as validation.
  if(response.length()>12){
    response.replace(0,11,"");
    response.replace(response.end()-1, response.end(),"");
    try {
      if(version) *version = std::stoi(response);
    } catch (...){
      std::cerr<<"SendConfig error: invalid response '"<<response<<"'"<<std::endl;
      return false;
    }
  } else {
    std::cerr<<"SendConfig error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

bool DAQInterface::SendConfig(const std::string& json_data, const std::string& author, const std::string& description, const std::string& device, unsigned int timestamp, int* version, const unsigned int timeout){
  
  if(version) *version=-1;
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+escape_json(name)+"\""
                         + ", \"author\":\""+escape_json(author)+"\""
                         + ", \"description\":\""+escape_json(description)+"\""
                         + ", \"data\":\""+escape_json(json_data)+"\" }";
  
  std::string response;
  std::string err="";
  
  if(!SendCommand("W_CONFIG", cmd_string, &response, &err, timeout)){
    std::cerr<<"SendConfig error: "<<err<<std::endl;
    return false;
  }
  
  // response is json with the version number of the created config entry
  // e.g. '{"version":3}'. check this is what we got, as validation.
  if(response.length()>12){
    response.replace(0,11,"");
    response.replace(response.end()-1, response.end(),"");
    try {
      if(version) *version = std::stoi(response);
    } catch (...){
      std::cerr<<"SendConfig error: invalid response '"<<response<<"'"<<std::endl;
      return false;
    }
  } else {
    std::cerr<<"SendConfig error: invalid response: '"<<response<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

// ===========================================================================
// Read Functions
// --------------

bool DAQInterface::GetCalibrationData(std::string& json_data, int version, const std::string& device, const unsigned int timeout){
  
  json_data = "";
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"device\":\""+escape_json(name)+"\""
                         + ", \"version\":\""+std::to_string(version)+"\" }";
  
  std::string err="";
  
  if(!SendCommand("R_CALIBRATION", cmd_string, &json_data, &err, timeout)){
    std::cerr<<"GetCalibrationData error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  return true;
  
}

bool DAQInterface::GetConfig(std::string& json_data, int version, const std::string& device, const unsigned int timeout){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"device\":\""+escape_json(name)+"\""
                         + ", \"version\":"+std::to_string(version) + "}";
  
  std::string err="";
  
  if(!SendCommand("R_CONFIG", cmd_string, &json_data, &err, timeout)){
    std::cerr<<"GetConfig error: "<<err<<std::endl;
    json_data = err;
    return false;
  }
  
  // response format '{"data":"<contents>"}' - strip out contents
  if(json_data.length()>11){
    json_data.replace(0,9,"");
    json_data.replace(json_data.end()-2, json_data.end(),"");
  } else {
    std::cerr<<"GetConfig error: invalid response: '"<<json_data<<"'"<<std::endl;
    return false;
  }
  
  return true;
  
}

bool DAQInterface::SQLQuery(const std::string& database, const std::string& query, std::vector<std::string>* responses, const unsigned int timeout){
  
  if(responses) responses->clear();
  
  const std::string& db = (database=="") ? m_dbname : database;
  
  const std::string command = "{ \"database\":\""+db+"\""
                            + ", \"query\":\""+escape_json(query)+"\" }";
  
  std::string err="";
  
  if(!SendCommand("R_QUERY", command, responses, &err, timeout)){
    std::cerr<<"SQLQuery error: "<<err<<std::endl;
    if(responses){
      responses->resize(1);
      responses->front() = err;
    }
    return false;
  }
  
  return true;
  
}

bool DAQInterface::SQLQuery(const std::string& query, const std::string& database, std::string* response, const unsigned int timeout){
  
  if(response) *response="";
  
  const std::string& db = (database=="") ? m_dbname : database;
  
  std::vector<std::string> responses;
  
  bool ok = !SQLQuery(db, query, &responses, timeout);
  
  if(response!=nullptr && responses.size()!=0){
    *response = responses.front();
    if(responses.size()>1){
      std::cerr<<"Warning: SQLQuery returned multiple rows, only first returned"<<std::endl;
    }
  }
  
  return ok;
}

// ===========================================================================
// Multicast Senders
// -----------------

bool DAQInterface::SendLog(const std::string& message, unsigned int severity, const std::string& device, unsigned int timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{\"time\":"+std::to_string(timestamp)
                         + ",\"device\":\""+escape_json(name)+"\""
                         + ",\"severity\":"+std::to_string(severity)
                         + ",\"message\":\"" + escape_json(message) + "\"}";
  
  std::string err="";
  
  if(!SendCommand(cmd_string, &err)){
    std::cerr<<"SendLog error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
}

bool DAQInterface::SendMonitoringData(const std::string& json_data, const std::string& device, unsigned int timestamp){
  
  const std::string& name = (device=="") ? m_name : device;
  
  std::string cmd_string = "{ \"time\":"+std::to_string(timestamp)
                         + ", \"device\":\""+escape_json(name)+"\""
                         + ", \"data\":\""+escape_json(json_data)+"\" }";
  
  std::string err="";
  
  if(!SendCommand(cmd_string, &err)){
    std::cerr<<"SendMonitoringData error: "<<err<<std::endl;
    return false;
  }
  
  return true;
  
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

bool DAQInterface::AddSlowControlVariable(std::string name, SlowControlElementType type, std::function<std::string(const char*)> function){
  
  return sc_vars.Add(name, type, function);
  
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

std::string DAQInterface::escape_json(std::string s){
  return s;
  
  // https://stackoverflow.com/a/27516892
  
  // i think the only thing we really need to worry about escaping here are double quotes,
  // which would prematurely terminate the string, and backslash, which may result in
  // the backslash and a subsequent character being converted to a special character;
  // e.g. '\n' -> line feed, or 0x5C 0x6E -> 0x0A, which alters the user's string.
  
  size_t pos=0;
  do {
    pos = s.find("\\",pos);
    if(pos!=std::string::npos){
      s.insert(pos,"\\");
      pos+=2;
    }
  } while(pos!=std::string::npos);
  
  pos=0;
  do {
    pos = s.find('"',pos);
    if(pos!=std::string::npos){
      s.insert(pos,"\\");
      pos+=2;
    }
  } while(pos!=std::string::npos);
  
  return s;
}


