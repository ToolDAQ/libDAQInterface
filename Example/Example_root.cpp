#include <iostream>
#include <DAQInterface.h>
#include <functional>

// this is the header you'll need to convert ROOT objects to JSON for sending to the database
#include "TBufferJSON.h"

// the rest are just what else is needed for this example
#include "TCanvas.h"
#include "TGraph.h"
#include "TH2.h"
#include "TF2.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TApplication.h"

/*
This is a minimal example showing how to send and retrieve ROOT plots
to the online database. See the full Example.cpp for a comprehensive set of features.
This example is split out such that the dependency on ROOT is optional.
Note that the class version is not included in the JSON, so schema evolution
is not supported, should ROOT make non-backwards-compatible changes to saved classes in the future.
*/

using namespace ToolFramework;

int main(int argc, const char** argv){
	
	////////////////////////////// setup /////////////////////////////////
	
	// as usual we need an instance of the DAQInterface class
	std::string device_name = "my_device"; //name of my device
	std::string Interface_configfile = "./InterfaceConfig";
	std::string database_name = "daq";
	
	std::cout<<"Constructing DAQinterface"<<std::endl;
	DAQInterface DAQ_inter;
	std::cout<<"Intialising (this may take a few seconds until the database connects)"<<std::endl;
	DAQ_inter.Init(device_name, Interface_configfile, database_name);  // initialiing DAQInterface
	
	// make a ROOT TApplication so that we can display graphs
	std::cout<<"Creating TApplication (only required for displaying graphs locally)"<<std::endl;
	TApplication root_app("myrootapp", &argc, const_cast<char**>(argv));
	
	//////////////////////////////////////////////////////////////////////
	
	DAQ_inter.sc_vars["Status"]->SetValue("Saving plots");
	
	std::cout<<"Creating a TGraph"<<std::endl;
	// make some dummy data
	std::vector<double> yvals(200);
	std::vector<double> xvals(yvals.size());
	for(int i=0; i<yvals.size(); ++i){
		xvals.at(i) = (5./double(yvals.size()))*double(i);
		yvals.at(i) = sin(xvals.at(i));
	}
	// make a TGraph from the data
	TGraph g(xvals.size(), xvals.data(), yvals.data());
	g.SetName("TotallyCoolSine");
	g.SetTitle("TotallyCoolSine;Time [dog years];Amplitude [units of sine]");
	g.SetLineColor(kMagenta);
	g.SetMarkerStyle(20);
	
	// convert the TGraph to JSON
	std::cout<<"Converting to JSON"<<std::endl;
	std::string graph_json = TBufferJSON::ToJSON(&g).Data();
	
	// we store the plot with a unique name and the draw options
	std::string graph_name = "TotallyCoolSine";
	std::string graph_draw_options = "ALP";
	
	// each time the same unique name is inserted, a new version of the plot is stored.
	// when saving a new version over TCP we may optionally pass a pointer to an integer,
	// which will be populated with the version number of the created entry.
	int graph_db_ver=0;
	
	// store it in the database
	bool persistent=true; // TODO
	// For now this setting only affects whether the request is sent over multicast (false, default) or tcp (true)
	// At a later date we may add the option for plots to only be stored in a temporary table.
	std::cout<<"Sending to database via TCP"<<std::endl;
	bool ok = DAQ_inter.SendROOTplot(graph_name, graph_draw_options, graph_json, persistent, &graph_db_ver);
	if(!ok){
		std::cerr<<"Error sending plot '"<<graph_name<<"' to database!"<<std::endl;
	} else {
		std::cout<<"Created plot '"<<graph_name<<"' version "<<graph_db_ver<<std::endl;
	}
	
	// Another example, this time storing a TH2.
	// Again we start by making a TH2 and filling it with some arbitrary data.
	std::cout<<"Creating a TH2D"<<std::endl;
	std::string histo_name = "CoolHistograph";
	TH2D histograffiti(histo_name.c_str(),"CoolWaves;Sideways [right];Leftways [up]",50,-10.,10.,50,-10.,10.);
	TF2 waveyfunc("wavefunc","abs(sin(x)/x)+abs(sin(y)/y)",-10,10,-10,10);
	histograffiti.FillRandom("wavefunc", 50000);
	histograffiti.Smooth();
	
	// convert to JSON with the same method as before
	std::cout<<"Converting to JSON"<<std::endl;
	std::string hist_json = TBufferJSON::ToJSON(&histograffiti).Data();
	
	// and specify the draw options
	std::string hist_draw_options = "colz";
	
	// send to the database. We omit the optional arguments this time.
	std::cout<<"Sending to database via multicast"<<std::endl;
	ok = DAQ_inter.SendROOTplot(histo_name, hist_draw_options, hist_json);
	if(!ok){
		std::cerr<<"Error sending plot '"<<histo_name<<"' to database!"<<std::endl;
	} else {
		std::cout<<"Sent OK"<<std::endl;
	}
	
	// ============================================================= //
	
	DAQ_inter.sc_vars["Status"]->SetValue("Retrieving plots");
	
	// we can likewise retrieve plots from the database and convert them back into ROOT objects
	std::string ret_graph_draw_options;
	std::string ret_graph_json;
	int ret_graph_version=-1; // -1 is latest version. Will be populated with the retreived version number.
	std::cout<<"Retreiving latest database entry with name '"<<graph_name<<"'"<<std::endl;
	ok = DAQ_inter.GetROOTplot(graph_name, ret_graph_version, ret_graph_draw_options, ret_graph_json);
	
	// create a new ROOT object from the retrieved JSON
	std::unique_ptr<TGraph> ret_graph;
	if(!ok){
		std::cerr<<"Error getting ROOT plot '"<<graph_name<<"' from database"<<std::endl;
	} else {
		std::cout<<"Got version "<<ret_graph_version<<", converting JSON to TGraph"<<std::endl;
		ret_graph = TBufferJSON::FromJSON<TGraph>(ret_graph_json.c_str());
		if(!ret_graph) std::cerr<<"Error creating TGraph from JSON '"<<ret_graph_json<<"'"<<std::endl;
		else std::cout<<"Conversion OK"<<std::endl;
	}
	
	if(ret_graph){
		/*
		// debug
		std::cout<<"TotallyCoolSine version "<<ret_graph_version<<" has "<<ret_graph->GetN()<<" points: [";
		for(int i=0; i<std::min(5,ret_graph->GetN()); ++i){
			if(i>0) std::cout<<", ";
			std::cout<<"{"<<ret_graph->GetX()[i]<<", "<<ret_graph->GetY()[i]<<"}";
		}
		if(ret_graph->GetN()>5) std::cout<<" ... ";
		std::cout<<" ]"<<std::endl;
		*/
	}
	
	// repeat for the histogram
	std::string ret_hist_draw_options;
	std::string ret_hist_json;
	int ret_hist_version=-1;
	std::cout<<"Getting latest version of plot "<<histo_name<<std::endl;
	ok = DAQ_inter.GetROOTplot(histo_name, ret_hist_version, ret_hist_draw_options, ret_hist_json);
	if(!ok) std::cerr<<"Failed to get ROOT plot '"<<histo_name<<"' from database!"<<std::endl;
	else std::cout<<"Converting to TH2"<<std::endl;
	
	// This time we use an alternative FromJSON signature using raw pointers instead of unique_ptr:
	// The previous method is preferred as it handles the lifetime of the ROOT object for you,
	// but if you do use this version be mindful that you are responsible for deleting the ROOT object.
	TH2* ret_hist = nullptr;
	if(ok) TBufferJSON::FromJSON(ret_hist, ret_hist_json.c_str());
	if(ok && !ret_hist) std::cerr<<"Error creating TH2 from JSON '"<<ret_hist_json<<"'"<<std::endl;
	else std::cout<<"Conversion OK"<<std::endl;
	
	// draw the retreived objects
	std::cout<<"Drawing retreived plots"<<std::endl;
	TCanvas* canv = new TCanvas("canv","Retrieved Plots",1024,400);
	canv->Divide(2,1);
	canv->cd(1);
	if(ret_graph) ret_graph->Draw(ret_graph_draw_options.c_str());
	canv->cd(2);
	if(ret_hist) ret_hist->Draw(ret_hist_draw_options.c_str());
	
	// wait until user closes the canvas
	DAQ_inter.sc_vars["Status"]->SetValue("Waiting for user to close canvas");
	std::cout<<"Waiting for user to close canvas"<<std::endl;
	while(gROOT->FindObject("canv")!=nullptr){
		canv->Modified();
		canv->Update();
		gSystem->ProcessEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	canv=nullptr; // deleted when the user closes it.
	
	// don't forget to delete the objects not encapsulated within a unique_ptr
	if(ret_hist) delete ret_hist;
	ret_hist = nullptr;
	
	// bonus aside:
	// if we're not sure what kind of object we need to make, we can read it from the json as follows
	if(ret_graph_json!=""){
		std::stringstream g_ss(ret_graph_json);
		std::string plot_type;
		while(getline(g_ss, plot_type)){
			size_t start_pos = plot_type.find("_typename");
			if(start_pos==std::string::npos) continue;
			//plot_type = plot_type.substr(17,(plot_type.length()-2)-17);   // returned strings strip out newlines
			//std::cout<<"found '_typename' at position "<<start_pos<<std::endl;
			start_pos = start_pos+13; // jump forward from '_typename' key to start of subsequent value
			size_t end_pos = plot_type.find('"',start_pos+1); // go from there to next closing quotation mark
			//std::cout<<"stripping "<<start_pos<<" to "<<end_pos<<std::endl;
			plot_type = plot_type.substr(start_pos, end_pos - start_pos);
			//std::cout<<"FOUND plot type: "<<plot_type<<std::endl;
			break;
		};
		std::cout<<"ROOT plot '"<<graph_name<<"' was of type '"<<plot_type<<"'"<<std::endl; // should be 'TGraph'
		// TODO perhaps one could implement a factory to create objects of common types...
	}
	
	DAQ_inter.sc_vars["Status"]->SetValue("Terminated");
	
	return 0;
	
}
