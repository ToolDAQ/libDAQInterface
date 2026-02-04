#include <iostream>
#include <DAQInterface.h>
#include <functional>

using namespace ToolFramework;

std::string Check(int ok){
	return ok ? "\033[32mPASS" : "\033[31mFAIL";
}

const std::string Reset = "\033[39m";

int main(){
	
	int verbose=1;
	
	std::string Interface_configfile = "./InterfaceConfig";
	
	DAQInterface DAQ_inter(Interface_configfile);
	//std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep for a little longer, connecting is slow
	std::string device_name = DAQ_inter.GetDeviceName();
	std::string tmp;
	bool ok;
	
	if(verbose) std::cout<<"Testing logging..."<<std::flush;
	ok = DAQ_inter.SendLog("test log message");
	if(!ok || verbose) std::cout<<"Logs sent: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Testing monitoring..."<<std::flush;
	ok = DAQ_inter.SendMonitoringData("{\"message\":\"test mon message\"}","general");
	if(!ok || verbose) std::cout<<"monitoring sent: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending test alarm..."<<std::flush;
	ok = DAQ_inter.SendAlarm("Test alarm");
	if(!ok || verbose) std::cout<<"Test alarm sent: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending test calibration data..."<<std::flush;
	ok = DAQ_inter.SendCalibrationData("{\"data\":[1,2,3]}","test calib data");
	if(!ok || verbose) std::cout<<"test calibration data sent: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"getting test calibration data..."<<std::flush;
	ok = DAQ_inter.GetCalibrationData(tmp, -1, device_name);
	if(!ok || verbose) std::cout<<"get calibration data: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending test device config..."<<std::flush;
	ok = DAQ_inter.SendDeviceConfig("{\"device settings\":[3,2,1]}","test user","test dev config");
	if(!ok || verbose) std::cout<<"test device config sent: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Getting test device config..."<<std::flush;
	ok = DAQ_inter.GetDeviceConfig(tmp, -1);
	if(!ok || verbose) std::cout<<"get device config: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending test run config..."<<std::flush;
	ok = DAQ_inter.SendRunConfig("{\""+device_name+"\":1}","test run","test user","test run config");
	if(!ok || verbose) std::cout<<"test run config sent: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Getting test run config by id..."<<std::flush;
	ok = DAQ_inter.GetRunConfig(tmp, 1);
	if(!ok || verbose) std::cout<<"get run config by id: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Getting test run config by name & version..."<<std::flush;
	ok = DAQ_inter.GetRunConfig(tmp, "test run",1);
	if(!ok || verbose) std::cout<<"get run config by name: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Getting test device config by run id..."<<std::flush;
	ok = DAQ_inter.GetDeviceConfigFromRunConfig(tmp, 1);
	if(!ok || verbose) std::cout<<"get device config by run config id: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Getting test device config by run name and version..."<<std::flush;
	ok = DAQ_inter.GetDeviceConfigFromRunConfig(tmp, "test run",1);
	if(!ok || verbose) std::cout<<"get device config by run config name & version: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending test PlotlyPlot..."<<std::flush;
	std::string trace = "{\"x\":[1,2,3],\"y\":[3,2,1]}";
	std::string layout = "{ \"title\":\"test plot\", \"xaxis\":{\"title\":\"x\"}, \"yaxis\":{\"title\":\"y\"} }";
	ok = DAQ_inter.SendPlotlyPlot("test_plot", trace, layout);
	if(!ok || verbose) std::cout<<"test plotly plot sent: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"getting test plotlyplot..."<<std::flush;
	trace=""; layout="";
	ok = DAQ_inter.GetPlotlyPlot("test_plot", trace, layout);
	if(!ok || verbose) std::cout<<"get plotly plot: "<<Check(ok)<<" = "<<trace<<", "<<layout<<Reset<<std::endl;
	
	// TODO add testing Get/Send Rootplot
	// requires either ROOT install, or hard-coded json that may not be small, or a test file
	
	if(verbose) std::cout<<"Testing single-record SQL query"<<Reset<<std::endl;
	ok = DAQ_inter.SQLQuery("SELECT time, message FROM logging ORDER BY time DESC LIMIT 1",tmp);
	if(!ok || verbose) std::cout<<"single-record SQL sent: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Testing multi-record SQL query"<<Reset<<std::endl;
	std::vector<std::string> resps;
	ok = DAQ_inter.SQLQuery("SELECT time, message FROM logging ORDER BY time DESC LIMIT 5",resps);
	if(!ok || verbose) std::cout<<"multi-record SQL sent: "<<Check(ok)<<", got "<<resps.size()<<" rows"<<Reset<<std::endl;
	if(ok && verbose) for(int i=0; i<resps.size(); ++i) std::cout<<"\t"<<i<<": '"<<resps.at(i)<<"'"<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending bad SQL query ..."<<Reset<<std::endl;
	ok = DAQ_inter.SQLQuery("SELECT potato, message FROM logging ORDER BY time DESC LIMIT 1",tmp);
	if(!ok || verbose) std::cout<<"bad SQL returned: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	return 0;
	
}
