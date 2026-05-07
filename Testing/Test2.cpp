#include <iostream>
#include <DAQInterface.h>
#include <functional>
#include <chrono>
#include <signal.h>

using namespace ToolFramework;

std::string Check(int ok){
	return ok ? "\033[32mPASS" : "\033[31mFAIL";
}

const std::string Reset = "\033[39m";
bool stopSignal=false;

void stopSignalHandler(int _ignored){
	// technically we could choose what to do based on the signal type passed, if we registered this function with multliple signals.
	stopSignal = true;
}

int main(){
	
	int verbose=1;
	std::string Interface_configfile = "./InterfaceConfig";
	
	std::cerr<<"making DAQ_inter"<<std::endl;
	DAQInterface DAQ_inter(Interface_configfile);
	std::cerr<<"made it"<<std::endl;
	DAQ_inter.SetVerbose(true);
	
	//std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep for a little longer, connecting is slow
	std::string device_name = DAQ_inter.GetDeviceName();
	std::string tmp;
	bool ok;
	
	Store m_variables;
	m_variables.Initialise("./Testing/Test2Config");
	int log_period_ms=0;
	int mon_period_ms=0;
	int alarm_period_ms=0;
	m_variables.Get("mon_period_ms",mon_period_ms);
	m_variables.Get("log_period_ms",log_period_ms);
	m_variables.Get("alarm_period_ms",alarm_period_ms);
	std::chrono::milliseconds mon_period{mon_period_ms};
	std::chrono::milliseconds log_period{log_period_ms};
	std::chrono::milliseconds alarm_period{alarm_period_ms};

	if(signal((int) SIGUSR1, stopSignalHandler) == SIG_ERR){
		std::cerr<<"Failed to setup SIGUSR1 handler!"<<std::endl;
		return 1;
	}
	
	std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
	std::chrono::time_point<std::chrono::steady_clock> start = now;
	std::chrono::time_point<std::chrono::steady_clock> last_log = now;
	std::chrono::time_point<std::chrono::steady_clock> last_mon = now;
	std::chrono::time_point<std::chrono::steady_clock> last_alarm = now;
	
	int logs_sent=0, mons_sent=0, alarms_sent=0;
	
	if(log_period_ms) std::cout<<"Sending log message once every "<<log_period_ms<<" ms"<<std::endl;
	if(mon_period_ms) std::cout<<"Sending monitoring message once every "<<mon_period_ms<<" ms"<<std::endl;
	if(alarm_period_ms) std::cout<<"Sending alarm once every "<<alarm_period_ms<<" ms"<<std::endl;
	
	while(!stopSignal){
		
		// logging
		now = std::chrono::steady_clock::now();
		if(log_period_ms && ((now-last_log)>log_period)){
			ok = DAQ_inter.SendLog("test log message");
			//ok = DAQ_inter.SendLog("test log message2"); // this prevents merging as only repeated like messages get merged
			std::clog<<"."<<std::flush;
			++logs_sent;
			last_log=now;
		}
		if(!ok){
			std::cout<<"Send Log: "<<Check(ok)<<Reset<<std::endl;
			break;
		}
		
		// monitoring
		now = std::chrono::steady_clock::now();
		if(mon_period_ms && ((now-last_mon)>mon_period)){
			ok = DAQ_inter.SendMonitoringData("{\"test\":\"mon message 1\"}","topic1");
			ok = DAQ_inter.SendMonitoringData("{\"test\":\"mon message 2\"}","topic2");
			// one of each of these is sent each 5s because they have unique topics
			std::clog<<"."<<std::flush;
			++mons_sent;
			last_mon=now;
		}
		if(!ok){
			std::cout<<"Send Mon: "<<Check(ok)<<Reset<<std::endl;
			break;
		}

		now = std::chrono::steady_clock::now();
		if(alarm_period_ms && ((now-last_alarm)>alarm_period)){
			ok = DAQ_inter.SendAlarm("test alarm message");
			//ok = DAQ_inter.SendAlarm("test alarm message2");
			std::clog<<"."<<std::flush;
			++alarms_sent;
			last_alarm=now;
		}
		if(!ok){
			std::cout<<"Send Alarm: "<<Check(ok)<<Reset<<std::endl;
			break;
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	now = std::chrono::steady_clock::now();
	std::cout<<"\nSent "<<logs_sent<<" logs, "<<mons_sent<<" mons, "<<alarms_sent<<" alarms in "
	         <<std::chrono::duration_cast<std::chrono::seconds>(now-start).count()<<" seconds"<<std::endl;
	
	return 0;
	
}
