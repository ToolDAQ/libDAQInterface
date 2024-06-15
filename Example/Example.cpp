#include <iostream>
#include <DAQInterface.h>
#include <functional>

using namespace ToolFramework;

// class for automating functions from slowcontrol
class AutomatedFunctions {
  
  public:
  
  AutomatedFunctions(DAQInterface* in_DAQ_inter){
    
    DAQ_inter=in_DAQ_inter;
    
  };
  
  DAQInterface* DAQ_inter;
  
  void new_event_func(const char* alert){
    
    DAQ_inter->SendLog("Hello i received a new_event alert");
    std::cout<<"recevied a 'new_event' alert"<<std::endl;
    
    //do new event alert action, e.g. reload configuration from DB
    
  }
  
  
  std::string start_func(const char* key){
    
    std::string tmp_msg="Device started";
    DAQ_inter->SendLog(tmp_msg);
    std::cout<<tmp_msg<<std::endl;
    
    return tmp_msg;
    
  }
  
  std::string voltage_change_func(const char* key){
    
    float new_value = DAQ_inter->sc_vars[key]->GetValue<float>();
    
    if(strcmp(key,"voltage_1")==0){
      // < add code here to change the hardware value of 'voltage_1' to new_value >
    } else if(strcmp(key,"voltage_3")==0){
      // < add code here to change the hardware value of 'voltage_3' to new_value >
    } else {
      std::string ret = std::string{"unknown key "}+key+" passed to callback 'voltage_change_func'";
      return ret;
    }
    
    std::string ret = "Changed "+std::string{key}+ " to new setpoint "+std::to_string(new_value);
    std::cout<<ret<<std::endl;
    return ret;
    
  }
  
  private:
  
};

int main(){
  
  int verbose=1;
  
  ////////////////////////////// setup /////////////////////////////////
  
  std::string device_name = "my_device"; //name of my device
  std::string Interface_configfile = "./InterfaceConfig";
  std::string database_name = "daq";
  
  std::cout<<"Constructing DAQInterface"<<std::endl;
  DAQInterface DAQ_inter;
  
  std::cout<<"Initialising DAQInterface. This may take ~15 seconds for the database to connect."<<std::endl;
  DAQ_inter.Init(device_name, Interface_configfile, database_name);  // initialiing DAQInterface
  
  std::cout<<"Constructing an AutomatedFunctions helper class to encapsulate callback functions"<<std::endl;
  AutomatedFunctions automated_functions(&DAQ_inter);
  // N.B. callback functions can simply be ordinary free functions, or they may be member functions
  // as in this case, where we define an 'AutomatedFunctions' helper class to hold them.
  // The advantage of using a helper class such as this is that the helper class can keep a handle
  // to the DAQInterface instance, which callback functions may then use to send logs, alarms,
  // update the service status, and so on.
  
  //////////////////////////////////////////////////////////////////////
  
  std::cout<<"Updating service status to 'Initialising'"<<std::endl;
  DAQ_inter.sc_vars["Status"]->SetValue("Initialising"); //setting status message
  
  ///////////////////////////////// logging alarms and alerts ///////////////////////////////
  
  std::cout<<"Testing logging..."<<std::flush;
  DAQ_inter.SendLog("unimportant message", 9, device_name);   //sending log message to database specifing severity and device name
  DAQ_inter.SendLog("important message", 0, device_name);
  DAQ_inter.SendLog("normal message");   // if not specified, the default severity level is 2 and the default name is the one passed to the DAQInterface constructor
  if(verbose) std::cout<<"Logs sent"<<std::endl;
  
  std::cout<<"Sending test alarm..."<<std::flush;
  DAQ_inter.SendAlarm("High current on channel 3"); // sending alarm message to database
  if(verbose) std::cout<<"Test alarm sent"<<std::endl;
  
  std::cout<<"Registering callback function 'AutomatedFunctions::new_event_func' to be invoked on alert 'new_event'..."<<std::flush;
  DAQ_inter.AlertSubscribe("new_event",  std::bind(&AutomatedFunctions::new_event_func, automated_functions,  std::placeholders::_1)); // if the DAQ sends out a global "new_event" alert, the registered callback function will be automatically invoked
  std::cout<<"Done"<<std::endl;
  
  ///////////////////////////////////////////////////////////////////////////
  
  std::cout<<"Updating service status to 'Ready'"<<std::endl;
  DAQ_inter.sc_vars["Status"]->SetValue("Ready");
  
  ///////////////////////////////////// slow control variables /////////////////////
  
  //defining slow control values
  std::cout<<"Registering slow controls"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'Info' variable..."<<std::flush;
  DAQ_inter.sc_vars.Add("Info",INFO);
  if(verbose) std::cout<<"Done\n\tSetting value to test arbitrary character support ..."<<std::flush;
  DAQ_inter.sc_vars["Info"]->SetValue(" hello this is an information message ,.!{}[]<>?/`~'@\" ");
  if(verbose) std::cout<<"Done"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'Start' button, linked to callback AutomatedFunctions::start_func ..."<<std::flush;
  DAQ_inter.sc_vars.Add("Start",BUTTON, std::bind(&AutomatedFunctions::start_func, automated_functions,  std::placeholders::_1));
  DAQ_inter.sc_vars["Start"]->SetValue(false);
  if(verbose) std::cout<<"Done"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'Stop' button..."<<std::flush;
  DAQ_inter.sc_vars.Add("Stop",BUTTON);
  DAQ_inter.sc_vars["Stop"]->SetValue(false);
  if(verbose) std::cout<<"Done"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'Quit' button..."<<std::flush;
  DAQ_inter.sc_vars.Add("Quit",BUTTON);
  DAQ_inter.sc_vars["Quit"]->SetValue(false);
  if(verbose) std::cout<<"Done"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'Power_on' radio button..."<<std::flush;
  DAQ_inter.sc_vars.Add("power_on",OPTIONS);
  DAQ_inter.sc_vars["power_on"]->AddOption("1");
  DAQ_inter.sc_vars["power_on"]->AddOption("0");
  DAQ_inter.sc_vars["power_on"]->SetValue("0");
  if(verbose) std::cout<<"Done"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'voltage_1' variable, linked to callback AutomatedFunctions::voltage_change_func ..."<<std::flush;
  DAQ_inter.sc_vars.Add("voltage_1", VARIABLE,  std::bind(&AutomatedFunctions::voltage_change_func, automated_functions,  std::placeholders::_1));  //example variable with automated function
  if(verbose) std::cout<<"Done\n\tConfiguring input range, step size and initial value..."<<std::flush;
  DAQ_inter.sc_vars["voltage_1"]->SetMin(0);
  DAQ_inter.sc_vars["voltage_1"]->SetMax(5000);
  DAQ_inter.sc_vars["voltage_1"]->SetStep(0.1);
  DAQ_inter.sc_vars["voltage_1"]->SetValue(3500.5);
  if(verbose) std::cout<<"\tDone"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'voltage_2' variable ..."<<std::flush;
  DAQ_inter.sc_vars.Add("voltage_2", VARIABLE); //example variable with no automated function
  if(verbose) std::cout<<"Done\n\tConfiguring input range, step size and initial value..."<<std::flush;
  DAQ_inter.sc_vars["voltage_2"]->SetMin(0);
  DAQ_inter.sc_vars["voltage_2"]->SetMax(5000);
  DAQ_inter.sc_vars["voltage_2"]->SetStep(10);
  DAQ_inter.sc_vars["voltage_2"]->SetValue(4000);
  if(verbose) std::cout<<"Done"<<std::endl;
  
  if(verbose) std::cout<<"\tRegistering 'voltage_3' variable, linked to callback 'AutomatedFunctions::voltage_change_func' ..."<<std::flush;
  // note that we can bind the same callback function to multiple slow controls.
  // The callback function receives a key to tell you which control was changed.
  DAQ_inter.sc_vars.Add("voltage_3", VARIABLE,  std::bind(&AutomatedFunctions::voltage_change_func, automated_functions,  std::placeholders::_1));
  if(verbose) std::cout<<"Done\n\tConfiguring input range, step size and initial value..."<<std::flush;
  DAQ_inter.sc_vars["voltage_3"]->SetMin(0);
  DAQ_inter.sc_vars["voltage_3"]->SetMax(5000);
  DAQ_inter.sc_vars["voltage_3"]->SetStep(0.1);
  DAQ_inter.sc_vars["voltage_3"]->SetValue(3800);
  if(verbose) std::cout<<"Done"<<std::endl;
  
  if(verbose) std::cout<<"All slow controls registered"<<std::endl;
  
  ////////////////////////////////////////////////////////////////////
  
  // local variables to retain last known values of slow controls
  bool power_on = false;
  float voltage_1 = 0;
  float voltage_2 = 0;
  float voltage_3 = 0;
  
  ////////////////////////////// configuration ///////////////////////
  
  Store configuration; //making ascii Store to hold and extract configuration variables
  std::string config_json=""; //string to hold json configuration
  int version=-1; //version of configuration to get; -1 to get the latest
  
  std::cout<<"Getting configuration settings from database"<<std::endl;
  bool ok = DAQ_inter.GetConfig(config_json, version); //get configuration from database
  
  // check we got a valid configuration back.
  if(!ok || config_json==""){
    
    std::cerr<<"No confguration for device "<<device_name<<", version "<<version<<" found"<<std::endl;
    // Actually, the first time this Example is run, the database will be empty,
    // so there will be no such configuration and this is expected.
    // Let's create an arbitrary configuration, and demonstrate entering it into the database.
    
    std::cout<<"Creating new configuration Store with default settings"<<std::endl;
    
    configuration.Set("power_on", power_on);   // Store::Set can be passed any non-class variable type
    configuration.Set("voltage_1", 0);         // it can accept constant values...
    configuration.Set("voltage_2", voltage_2); // ... or local variables ...
    configuration.Set("voltage_3", DAQ_inter.sc_vars["voltage_3"]->GetValue<float>()); // ... or the value returned by function call
    
    // this last line also demonstrates one method of retreiving the current value of a slow control -
    // by specifying the return type in the GetValue call, the current value is returned:
    voltage_2 = DAQ_inter.sc_vars["voltage_2"]->GetValue<float>();
    // alternatively we can pass an appropriate variable to the GetValue call, which will set its value:
    DAQ_inter.sc_vars["voltage_2"]->GetValue(voltage_2);
    
    if(verbose) std::cout<<"Converting Store to JSON"<<std::endl;
    configuration>>config_json; //output current configuration to json
    
    if(verbose) std::cout<<"sending configuration json '"<<config_json<<"' to database"<<std::endl;
    DAQ_inter.SendConfig(config_json, "John Doe", "My New Config"); //uplaod configuration to database
    
  } else{
    
    // the second time we run this example, the saved configuration will exist and be returned.
    std::cout<<"Got configuration version "<<version<<std::endl;
    configuration.JsonParser(config_json); //loading json into variable store
    
    if(verbose){
      std::cout<<"Configuration parameters are:\n------ "<<std::endl;
      configuration.Print(); //print the current configuration
      std::cout<<"------"<<std::endl;
    }
    
    if(verbose) std::cout<<"Initialising slow control settings from retreived configuration"<<std::endl;
    // we can retreive settings from the store into local variables...
    configuration.Get("power_on", power_on);
    // ... and then use the local variables to update the slow control values
    DAQ_inter.sc_vars["power_on"]->SetValue(power_on);
   
   // alternatively we could bypass the local variable and pass the result straight to the slow control
   // (note that we need to specify the variable type in the 'Get' call in this case)
    DAQ_inter.sc_vars["voltage_1"]->SetValue(configuration.Get<float>("voltage_1"));
    
    // Note that Store::Get returns false if the requested key does not exist.
    // We can use this to fall back to a default value if there is no stored setting as follows:
    if(configuration.Get("voltage_2", voltage_2) == false) voltage_2 = 2000;
    DAQ_inter.sc_vars["voltage_2"]->SetValue(voltage_2);
    
    // or we may take alternative action such as logging an error
    if(!configuration.Get("voltage_3", voltage_3)) DAQ_inter.SendLog("voltage3 not set", 0, device_name); //sends log message if not in configuration
    else DAQ_inter.sc_vars["voltage_3"]->SetValue(voltage_3);
    
  }
  
  ////////////////////////////////////////////////////////////////////
  
  //////////////////////////////// creating monitoring store //////////////
  // Monitoring data needs to be formatted as a JSON string.
  // As we've just seen the Store class provides a convenient way to generate a JSON string that
  // encapsulates an arbitrary set of variables
  
  Store monitoring_data; // sorage object for monitoring vales;
  
  /////////////////////////////////////////////////////////////////
  
  /////////////////////////// program operation //////////////////////
  
  bool running=true;
  bool last_started=true;
  bool started=false;
  
  std::cout<<"Beginning outer program loop"<<std::endl;
  while(running){ /// outer program loop, runs until user clicks 'Quit' slow control
    
    if(last_started==true && started==false) std::cout<<"Waiting for user to click 'Start' or 'Quit'..."<<std::endl;
    
    //  Example of checking slow control values manually
    //  note: the below section could equivalently be moved to AutomatedFunctions::Start_func()
    started = DAQ_inter.sc_vars["Start"]->GetValue<bool>();
    if(started){
      std::cout<<"Beginning inner program loop"<<std::endl;
      
      DAQ_inter.sc_vars["Status"]->SetValue("Running");
      DAQ_inter.sc_vars["Start"]->SetValue(false);  // important! reset the slow control value after use.
      
      DAQ_inter.sc_vars.AlertSend("new_event"); // example of sending alert to all DAQ devices
      
    }
    last_started = started;
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    
    while(started){ // program operation loop, runs until user clicks 'Stop' slow control (or 'Quit')
      
      
      running=(!DAQ_inter.sc_vars["Quit"]->GetValue<bool>()); 
      
      if(DAQ_inter.sc_vars["Stop"]->GetValue<bool>() || !running){
        std::cout<<"Inner program loop stopped"<<std::endl;
        started =false;
        DAQ_inter.sc_vars["Status"]->SetValue("Stopped");
        DAQ_inter.sc_vars["Stop"]->SetValue(false);
        DAQ_inter.sc_vars["Start"]->SetValue(false);
      }
      
      
      //////////////////////////// monitoring usage //////////////////////////
      
      // Clear the Store of monitoring data.
      // This ensures stale values aren't carried over.
      // Note this isn't strictly necessary - subsequent 'Set' calls will update the internally held vlaues,
      // so if you want other entries to persist even if they haven't been updated, skip this call.
      monitoring_data.Delete();
      
      // put all the variables to be monitored into the monitoring Store...
      monitoring_data.Set("temp_1", 30+(rand()%100)/100.);  // values recorded in monitoring store
      monitoring_data.Set("temp_2", 28+(rand()%100)/100.);
      monitoring_data.Set("temp_3", 18+(rand()%100)/100.);
      monitoring_data.Set("current_1", rand()%10/2.);
      monitoring_data.Set("current_2", rand()%10/2.);
      monitoring_data.Set("current_3", rand()%10/2.);
      // note; here we're sending monitoring values that represent the current value
      // of slow controls - i.e., setpoints. In a real application we would probably
      // send values read from the hardware.
      monitoring_data.Set("voltage_1", DAQ_inter.sc_vars["voltage_1"]->GetValue<float>());
      monitoring_data.Set("voltage_2", DAQ_inter.sc_vars["voltage_2"]->GetValue<float>());
      monitoring_data.Set("voltage_3", DAQ_inter.sc_vars["voltage_3"]->GetValue<float>());
      monitoring_data.Set("power_on", DAQ_inter.sc_vars["power_on"]->GetValue<int>());
      
      // convert to JSON
      std::string monitoring_json="";
      monitoring_data>>monitoring_json; /// prducing monitoring json 
      
      // send to the database
      DAQ_inter.SendMonitoringData(monitoring_json);
      
      //////////////////////////////////////////////////////////////////////////////////////////
      
      
      ///////////////////////  using and getting slow control values /////////////// 
      
      // get the current value of the 'power_on' control
      bool last_power_on = power_on;
      DAQ_inter.sc_vars["power_on"]->GetValue(power_on);
      
      if(!power_on){
        if(last_power_on && !power_on) std::cout<<"Power turned off"<<std::endl;
        /*
        if(DAQ_inter.sc_vars["voltage_1"]->GetValue<bool>()) DAQ_inter.sc_vars["voltage_1"]->SetValue(0.0); 
        if(DAQ_inter.sc_vars["voltage_2"]->GetValue<bool>()) DAQ_inter.sc_vars["voltage_2"]->SetValue(0.0);
        if(DAQ_inter.sc_vars["voltage_3"]->GetValue<bool>()) DAQ_inter.sc_vars["voltage_3"]->SetValue(0.0);
        */
      } else if(last_power_on!=power_on){
        std::cout<<"Power turned on"<<std::endl;
      }
      last_power_on = power_on;
      
      // voltage 1 & 3 are changed automatically via automated_functions.voltage_change,
      // but here is an example for voltage 2 of polling for changes and responding to them manually
      
      // compare with last known voltage using local variable
      if(DAQ_inter.sc_vars["voltage_2"]->GetValue<float>() != voltage_2 ){
        
        // on change, update the local voltage_2 variable...
        DAQ_inter.sc_vars["voltage_2"]->GetValue(voltage_2);
        std::cout<<"Voltage_2 updated to "<<voltage_2<<" V"<<std::endl;
        
        // ... and enact the corresponding change in voltage
        // < add code here to change the appropriate hardware settings to apply the new value of voltage_2 >
        
      }
      
      ////////////////////////////////
      
      sleep(1);
      
    } // end of operation loop
    
    // check if the quit button has been pressed
    running=(!DAQ_inter.sc_vars["Quit"]->GetValue<bool>()); 
    
    usleep(1000);
    
  } // end of program loop
  
  
  std::cout<<"Application terminated"<<std::endl;
  DAQ_inter.sc_vars["Status"]->SetValue("Terminated");
  
  return 0;
  
}
