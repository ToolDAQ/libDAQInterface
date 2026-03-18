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
	DAQ_inter.SetVerbose(true);
	//std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep for a little longer, connecting is slow
	std::string device_name = DAQ_inter.GetDeviceName();
	std::string tmp;
	bool ok;
	
	if(verbose) std::cout<<"Testing logging..."<<std::flush;
	ok = DAQ_inter.SendLog("test log message");
	if(!ok || verbose) std::cout<<"Send Log: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Testing monitoring..."<<std::flush;
	ok = DAQ_inter.SendMonitoringData("{\"message\":\"test mon message\"}","general");
	if(!ok || verbose) std::cout<<"Send monitoring: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending test alarm..."<<std::flush;
	ok = DAQ_inter.SendAlarm("Test alarm");
	if(!ok || verbose) std::cout<<"Send alarm: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Sending test calibration data..."<<std::flush;
	ok = DAQ_inter.SendCalibrationData("{\"data\":[1,2,3]}","test calib data");
	if(!ok || verbose) std::cout<<"Send calibration data: "<<Check(ok)<<Reset<<std::endl;
	
	// FIXME add checks that returned data returns what's expected
	
	if(verbose) std::cout<<"getting test calibration data..."<<std::flush;
	ok = DAQ_inter.GetCalibrationData(tmp, -1, device_name);
	if(!ok || verbose) std::cout<<"Get calibration data: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	// to be able to send a device config we need to have a corresponding device.
	// libDAQInterface doesn't provide a function to create a device, but we can do it manually
	ok = DAQ_inter.SQLQuery("INSERT INTO devices ( name ) VALUES ( '"+device_name+"' ) ON CONFLICT DO NOTHING",tmp);
	if(!ok) std::cout<<Check(ok)<<"Error inserting test device: "<<tmp<<Reset<<std::endl;
	
	// these may fail if the above failed... but we don't expect it to...
	if(verbose) std::cout<<"Sending test device config..."<<std::flush;
	ok = DAQ_inter.SendDeviceConfig("{\"device settings\":[3,2,1]}","test user","test dev config");
	if(!ok || verbose) std::cout<<"Send device config: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Getting test device config..."<<std::flush;
	ok = DAQ_inter.GetDeviceConfig(tmp, -1);
	if(!ok || verbose) std::cout<<"Get device config: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	/* - no longer in libDAQInterface - use the web interface
	if(verbose) std::cout<<"Sending test run config..."<<std::flush;
	ok = DAQ_inter.SendXXXConfig("{\""+device_name+"\":1}","test run","test user","test run config");
	if(!ok || verbose) std::cout<<"Send run config: "<<Check(ok)<<Reset<<std::endl;
	*/
	
	// again we do this manually
	std::string query;
	int base_id=-1;
	int runmode_id=-1;
	Store mystore;
	try {
		query = "DO $$ BEGIN IF NOT EXISTS ( SELECT name FROM base_config WHERE name='"+device_name+"' AND version=1 ) THEN "
		        "INSERT INTO base_config ( name, author, description, data ) values ( '"+device_name+"', 'test', 'test', '{\""+device_name+"\":2}' ); "
		        "END IF; END $$ ";
		ok = DAQ_inter.SQLQuery(query,tmp);
		if(!ok) std::cerr<<"error inserting base config"<<std::endl;
		query = "SELECT config_id FROM base_config WHERE name='"+device_name+"' AND version=1";
		ok = DAQ_inter.SQLQuery(query,tmp);
		if(ok){
			mystore.JsonParser(tmp);
			mystore.Get("config_id",base_id);
			if(!ok) std::cerr<<"no base config_id in "<<tmp<<std::endl;
			mystore.Delete();
		} else {
			std::cerr<<"error fetching base_config_id"<<std::endl;
		}
		query = "DO $$ BEGIN IF NOT EXISTS ( SELECT name FROM runmode_config WHERE name='"+device_name+"' AND version=1 ) THEN "
		        "INSERT INTO runmode_config ( name, author, description, data ) values ( '"+device_name+"', 'test', 'test', '{\""+device_name+"\":3}' ); "
		        "END IF; END $$ ";
		ok = DAQ_inter.SQLQuery(query,tmp);
		if(!ok) std::cerr<<"error inserting runmode config"<<std::endl;
		query = "SELECT config_id FROM runmode_config WHERE name='"+device_name+"' AND version=1";
		ok = DAQ_inter.SQLQuery(query,tmp);
		if(ok){
			mystore.JsonParser(tmp);
			ok = mystore.Get("config_id",runmode_id);
			if(!ok) std::cerr<<"no runmode config_id in "<<tmp<<std::endl;
			mystore.Delete();
		} else {
			std::cerr<<"error fetching runmode_config_id"<<std::endl;
		}
		
	} catch(std::exception& e){
		std::cout<<"caught "<<e.what()<<" with tmp "<<tmp<<std::endl;
	}
	
	if(verbose) std::cout<<"Getting test run config by id {"<<base_id<<","<<runmode_id<<"}..."<<std::flush;
	ok = DAQ_inter.GetRunConfig(tmp, base_id, runmode_id);
	if(!ok || verbose) std::cout<<"Get run config (by id): "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	// N.B: this is not a full detector configuration
	if(verbose) std::cout<<"Getting test runmode config by name & version..."<<std::flush;
	ok = DAQ_inter.GetRunModeConfig(tmp, device_name, 1);
	if(!ok || verbose) std::cout<<"Get run config (by name): "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Getting test device config by run id {"<<base_id<<","<<runmode_id<<"}..."<<std::flush;
	ok = DAQ_inter.GetDeviceConfigFromRunConfig(tmp,  base_id, runmode_id);
	if(!ok || verbose) std::cout<<"Get device config (by run config id): "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	/* - not currently supported, no plans to reimplement presently.
	if(verbose) std::cout<<"Getting test device config by run name and version..."<<std::flush;
	ok = DAQ_inter.GetDeviceConfigFromRunConfig(tmp, "test run",1);
	if(!ok || verbose) std::cout<<"Get device config (by run config name & version): "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	*/
	
	if(verbose) std::cout<<"Sending test PlotlyPlot..."<<std::flush;
	std::string trace = "{\"x\":[1,2,3],\"y\":[3,2,1]}";
	std::string layout = "{ \"title\":\"test plot\", \"xaxis\":{\"title\":\"x\"}, \"yaxis\":{\"title\":\"y\"} }";
	ok = DAQ_inter.SendPlotlyPlot("test_plot", trace, layout);
	if(!ok || verbose) std::cout<<"Send plotly plot: "<<Check(ok)<<Reset<<std::endl;
	
	if(verbose) std::cout<<"getting test plotlyplot..."<<std::flush;
	trace=""; layout="";
	ok = DAQ_inter.GetPlotlyPlot("test_plot", trace, layout);
	if(!ok || verbose) std::cout<<"Get plotly plot: "<<Check(ok)<<" = "<<trace<<", "<<layout<<Reset<<std::endl;
	
	// TODO add testing Get/Send Rootplot
	// requires either ROOT install, or hard-coded json that may not be small, or a test file
	
	if(verbose) std::cout<<"Testing single-record SQL query"<<Reset<<std::endl;
	ok = DAQ_inter.SQLQuery("SELECT time, message FROM logging ORDER BY time DESC LIMIT 1",tmp);
	if(!ok || verbose) std::cout<<"Get single record via SQL: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	if(verbose) std::cout<<"Testing multi-record SQL query"<<Reset<<std::endl;
	std::vector<std::string> resps;
	ok = DAQ_inter.SQLQuery("SELECT time, message FROM logging ORDER BY time DESC LIMIT 5",resps);
	if(!ok || verbose) std::cout<<"Get multiple records via SQL: "<<Check(ok)<<", got "<<resps.size()<<" rows"<<std::endl;
	if(ok && verbose){
		for(int i=0; i<resps.size(); ++i) std::cout<<"\t"<<i<<": '"<<resps.at(i)<<"'"<<std::endl;
		std::cout<<Reset;
	}
	
	if(verbose) std::cout<<"Sending bad SQL query ..."<<Reset<<std::endl;
	ok = DAQ_inter.SQLQuery("SELECT potato, message FROM logging ORDER BY time DESC LIMIT 1",tmp);
	if(!ok || verbose) std::cout<<"Running bad SQL query returned: "<<Check(ok)<<" = "<<tmp<<Reset<<std::endl;
	
	return 0;
	
}
