# example python script
# ---------------------
import time # for sleep
import random # for random
# import cppyy and ctypes for interaction with c++ entities
import cppyy, cppyy.ll
import ctypes
# streamline accessing std namespace
std = cppyy.gbl.std
# pull in classes from the libDAQInterface
cppyy.load_reflection_info('libDAQInterfaceClassDict')
from cppyy.gbl import ToolFramework
from cppyy.gbl.ToolFramework import DAQInterface

class AutomatedFunctions:
  DAQ_inter = 0
  name=0
  
  def __init__(self, iDAQ_inter, iname):
    self.DAQ_inter = iDAQ_inter
    self.name = iname
  
  def new_event_func(self, event_name: str, event_payload: str) -> None:
    print("new_event_func fired for event ",event_name)
    self.DAQ_inter.SendLog(self.name+" received an alert for event "+event_name)
    # add your desired actions on new_event here
    # if this function is subscribed to multiple alerts, you can use event_name
    # to determine the appropriate actions to take
    # return type for AlertSubscribe functions is void (no return value)
  
  def start_func(self, control_name: str) -> str:
    self.DAQ_inter.SendLog(self.name," received start signal")
    # add code to perform any startup actions here
    ret = "Service Started"
    return ret
  
  def change_voltage(self, control_name: str) -> str:
    print("voltage change callback for control ",control_name)
    # the same function can be registered as a callback for multiple controls;
    # the changed control name is received as an argument.
    # so e.g. this function may be registered to controls for multiple HV channels
    # (each with a unique name), with the name received identifying the channel altered.
    # get the new value of the control (i.e. the requested voltage)
    new_voltage = self.DAQ_inter.sc_vars[control_name].GetValue['float']()
    # add appropriate code to enact changing the voltage here
    msg = self.name + " setting voltage of "+control_name+" to "+str(new_voltage)
    self.DAQ_inter.SendLog(msg)
    # the function should return a string, for example indicating success/failure
    ret = control_name+" set to "+str(new_voltage)+"V"
    return ret
  

if __name__ == "__main__":
  
  # configuration
  device_name = "my_device"
  PGClient_configfile = "./PGClientConfig"
  database_name = "daq"
  
  print("Initialising daqinterface")
  # initialise DAQInterface
  DAQ_inter = DAQInterface()
  DAQ_inter.Init(device_name, PGClient_configfile, database_name)
  automated_functions = AutomatedFunctions(DAQ_inter, device_name)
  
  # set initial status so we can track when the service is ready
  DAQ_inter.sc_vars["Status"].SetValue("Initialising")
  
  ###############################################################
  ####### demonstrate sending logging messages and alarms #######
  ###############################################################
  
  print("Testing logging")
  # send a log message to the database, specifing severity and device name
  # severity 0 is critical, higher numbers => lower severity
  DAQ_inter.SendLog("important message", 0, device_name)
  DAQ_inter.SendLog("unimportant message", 9, device_name)
  # we may omit the severity and/or logging source, in which case a default severity of 2 will be used,
  # and the logging source will be the device name we passed to the DAQInterface constructor
  DAQ_inter.SendLog("normal message")
  # the signature for sending alarms is the same as logging messages
  DAQ_inter.SendAlarm("High current on channel 3", 0, device_name)
  
  ###############################################################
  #######        registering subscriber functions         #######
  ###############################################################
  
  print("Testing AlertSubscribe")
  # we can register functions to be invoked in response to a broadcast event,
  # such as the start of a new run or when the program state changes (an example is given later).
  # we register a function with the DAQInterface::AlertSubscribe method.
  # for python we must pass a variable assigned to the desired function,
  # and that variable must still be in scope at any time it may be invoked.
  # i.e. we cannot pass 'automated_functions.new_event_func' directly as per:
  #DAQ_inter.AlertSubscribe("new_event", automated_functions.new_event_func)
  # but instead we must do this:
  new_event_func_ref = automated_functions.new_event_func
  DAQ_inter.AlertSubscribe("new_event", new_event_func_ref)
  
  ###############################################################
  #######           registering slow controls             #######
  ###############################################################
  
  print("Adding controls")
  # We can register controls associated with our service via the DAQ_inter.sc_vars.Add method
  # this takes a control name and type, as a minimum.
  # available types are: { BUTTON, VARIABLE, OPTIONS, COMMAND, INFO }
  DAQ_inter.sc_vars.Add("Info",cppyy.gbl.ToolFramework.INFO)
  
  # Use DAQ_inter.sc_vars["ControlName"] to access a SlowControlElement.
  # Use the SetValue method of a control to update its value displayed on the webpage
  DAQ_inter.sc_vars["Info"].SetValue(" hello this is an information message ,.!{}[]<>?/`~'@\" ")
  
  # Typical controls might include buttons for starting, stopping and quitting a service
  DAQ_inter.sc_vars.Add("Start",cppyy.gbl.ToolFramework.BUTTON)
  DAQ_inter.sc_vars["Start"].SetValue(False)
  
  DAQ_inter.sc_vars.Add("Stop",cppyy.gbl.ToolFramework.BUTTON)
  DAQ_inter.sc_vars["Stop"].SetValue(False)
  
  DAQ_inter.sc_vars.Add("Quit",cppyy.gbl.ToolFramework.BUTTON)
  DAQ_inter.sc_vars["Quit"].SetValue(False)
  
  # We can make a radio button control with the OPTIONS control type
  DAQ_inter.sc_vars.Add("power_on",cppyy.gbl.ToolFramework.OPTIONS)
  DAQ_inter.sc_vars["power_on"].AddOption("1")
  DAQ_inter.sc_vars["power_on"].AddOption("0")
  DAQ_inter.sc_vars["power_on"].SetValue("0")
  
  # we can add variable controls with the VARIABLE control type
  # this additionally takes a range and step size
  DAQ_inter.sc_vars.Add("voltage_1", cppyy.gbl.ToolFramework.VARIABLE)
  DAQ_inter.sc_vars["voltage_1"].SetMin(0)
  DAQ_inter.sc_vars["voltage_1"].SetMax(5000)
  DAQ_inter.sc_vars["voltage_1"].SetStep(0.1)
  DAQ_inter.sc_vars["voltage_1"].SetValue['float'](3500.5)
  
  
  
  ###############################################################
  #######           querying control values               #######
  ###############################################################
  
  print("Querying controls")
  # we can retrieve control values either by passing a reference variable of suitable type...
  voltage_1 = ctypes.c_float()
  DAQ_inter.sc_vars["voltage_1"].GetValue(voltage_1)
  # ... or by specifying the type explicitly, to receive it as a return value
  voltage_1 = DAQ_inter.sc_vars["voltage_1"].GetValue['float']()
  
  ###############################################################
  #######           registering callbacks                 #######
  ###############################################################
  
  print("Registering control callbacks")
  # similar to subscriber functions, we can register a callback function to be invoked
  # whenever a control value is changed. This enables us to act on changes when they happen
  # without the need to constantly poll the DAQInterface to identify when something changes.
  change_voltage_ref = automated_functions.change_voltage
  DAQ_inter.sc_vars.Add("voltage_2", cppyy.gbl.ToolFramework.VARIABLE, change_voltage_ref)
  DAQ_inter.sc_vars["voltage_2"].SetMin(0)
  DAQ_inter.sc_vars["voltage_2"].SetMax(5000)
  DAQ_inter.sc_vars["voltage_2"].SetStep(10)
  DAQ_inter.sc_vars["voltage_2"].SetValue(4000)
  
  ###############################################################
  #######  retrieving configurations from the database    #######
  ###############################################################
  
  # configuration entries are uniquely identified by a pair of {device name, version number}
  # and are represented in the database as JSON strings. To query for this device's configuration:
  print("Querying DB")
  version=-1  # use version -1 to get the latest version
  config_json=std.string("")
  ok = DAQ_inter.GetConfig(config_json, version)
  
  # a Store class instance can parse a JSON string to more easily access settings
  # and can generate a JSON string from contents set by a series of simple 'Set' calls
  # so is useful both for reading and writing configuration entries
  configuration = cppyy.gbl.ToolFramework.Store()
  
  # local variables to retain current values
  power_on = ctypes.c_bool()
  voltage_1 = ctypes.c_float()
  voltage_2 = ctypes.c_float()
  
  # check if a configuration was found in the database
  if not ok or config_json=="":
    
    print("Making new DB entry")
    # no matching configuration entry was found. let's make one.
    power_on.value = False  # IMPORTANT: when changing values of ctypes variables use '.value'
                            # doing 'power_on = False' will change the type of the power_on variable
                            # and may lead to errors when passed to later Get/Set calls!
    configuration.Set("power_on", power_on)
    configuration.Set("voltage_1", DAQ_inter.sc_vars["voltage_1"].GetValue['float']())
    configuration.Set("voltage_2", 4000)
    
    configuration.__rshift__['std::string'](config_json)
    
    # uplaod configuration to the database
    print("sending new config_json: '",config_json,"'")
    DAQ_inter.SendConfig(config_json, "DemoAuthor", "Demo Description")
    
  else:
    
    print("Parsing query response")
    # we got a configuration JSON string. Parse it with the Store class
    print("got config JSON: '",config_json,"'")
    configuration.JsonParser(config_json)
    
    # print the configuration
    configuration.Print()
    
    # retrieve the configuration variables and use them to set initial control values
    configuration.Get("power_on", power_on)
    DAQ_inter.sc_vars["power_on"].SetValue(power_on)
    DAQ_inter.sc_vars["voltage_1"].SetValue['float'](configuration.Get['float']("voltage_1"))
    
    # if the Store does not contain a given key
    # (i.e. no corresponding setting was in the database configuration entry)
    # the Store::Get method will return False
    if configuration.Get("voltage_2", voltage_2) == True:
      DAQ_inter.sc_vars["voltage_2"].SetValue['float'](voltage_2)
    else:
      # use a default value if no setting is available
      voltage_2 = 2000
      # and report this to the logging database
      DAQ_inter.SendLog("voltage_2 not set in configuration version "+str(version), 0, device_name)
    
  
  ###############################################################
  #######                  Main Program Loop              #######
  ###############################################################
  
  print("main loop")
  # We'll use a Store to accumulate monitoring data
  # and convert to json to send to the webpage for plotting
  monitoring_data = cppyy.gbl.ToolFramework.Store()
  
  running = True
  DAQ_inter.sc_vars["Status"].SetValue("Ready")
  
  while running:
    
    running=(not DAQ_inter.sc_vars["Quit"].GetValue['bool']())
    
    # check for Start control being clicked
    #######################################
    started = DAQ_inter.sc_vars["Start"].GetValue['bool']()
    if started:
      print("Got Start signal")
      # (n.b. we could have alternatively defined these actions in automated_functions.start_func)
      # update the status control to indicate our new program state
      DAQ_inter.sc_vars["Status"].SetValue("Running")
      
      # fire all functions subscribed to the 'new_event' signal
      print("sending 'new_event' alert")
      DAQ_inter.sc_vars.AlertSend("new_event")
      
      # reset the 'Start' button, so that we can accept new presses
      DAQ_inter.sc_vars["Start"].SetValue(False)
      
    
    while started:
      
      # check for Quit control being clicked
      #######################################
      running = (not DAQ_inter.sc_vars["Quit"].GetValue['bool']())
      
      # if clicked, update our status indicator and reset the control buttons
      if DAQ_inter.sc_vars["Stop"].GetValue['bool']() or not running:
        print("Got stop signal")
        started = False
        DAQ_inter.sc_vars["Status"].SetValue("Stopped")
        DAQ_inter.sc_vars["Stop"].SetValue(False)
        DAQ_inter.sc_vars["Start"].SetValue(False)
      
      # Report current values to monitoring plots
      ###########################################
      
      # clear any values from the monitoring Store
      monitoring_data.Delete()
      # Note that 'Store::Set' calls to existing keys will overwrite old values, so calling 'Delete'
      # just ensures no entries that weren't 'Set' before it's invoked carry over.
      # Of course in the specific case here we make the same series of 'Set' calls on every loop,
      # so calling 'Delete' is redundant as all keys will either be overwritten or re-created.
      
      # populate the monitoring Store
      print("setting monitoring vals")
      monitoring_data.Set("temp_1", 30+random.random())
      monitoring_data.Set("temp_2", 28+random.random())
      monitoring_data.Set("temp_3", 18+random.random())
      monitoring_data.Set("current_1", random.uniform(0,5))
      monitoring_data.Set("current_2", random.uniform(0,5))
      monitoring_data.Set("voltage_1", DAQ_inter.sc_vars["voltage_1"].GetValue['float']())
      monitoring_data.Set("voltage_2", DAQ_inter.sc_vars["voltage_2"].GetValue['float']())
      monitoring_data.Set("power_on", DAQ_inter.sc_vars["power_on"].GetValue['int']())
      
      # generate a JSON from the contents
      monitoring_json = std.string("")
      monitoring_data.__rshift__['std::string'](monitoring_json)
      
      # send to the Database for plotting on the webpage
      DAQ_inter.SendMonitoringData(monitoring_json)
      
      # retrieve and respond to control changes
      ###########################################
      print("checking for updated controls")
      DAQ_inter.sc_vars["power_on"].GetValue(power_on)
      #power_on = DAQ_inter.sc_vars["power_on"].GetValue['bool'](); # or this
      if not power_on:
        if DAQ_inter.sc_vars["voltage_1"].GetValue['bool']():
          DAQ_inter.sc_vars["voltage_1"].SetValue['float'](0.0)
        if DAQ_inter.sc_vars["voltage_2"].GetValue['bool']():
          DAQ_inter.sc_vars["voltage_2"].SetValue['float'](0.0)
      
      # voltage_2 is changed automatically via automated_functions.voltage_change
      # but here is an example for voltage_1 of doing it manually:
      # get the current control value from DAQ_inter, and compare with last known setting
      if DAQ_inter.sc_vars["voltage_1"].GetValue['float']() != voltage_1.value:
        
        # if the control value has changed, update the local variable holding last known value
        DAQ_inter.sc_vars["voltage_1"].GetValue(voltage_1)
        print("voltage_1 change requested; new value is ",voltage_1)
        
        # and enact the change in hardware
        # [suitable code here]
      
      # limit loop rate so we don't lock up the CPU
      #############################################
      #print("inner sleep")
      time.sleep(1)
    
    # end of operation loop
    
    #print("outer sleep")
    time.sleep(1)
  
  # end of program loop
  print("end main loop")
  
  DAQ_inter.sc_vars["Status"].SetValue("Terminated")
