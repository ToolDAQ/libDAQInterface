Dependencies=./Dependencies

ZMQLib= -L $(Dependencies)/zeromq-4.0.7/lib -lzmq
ZMQInclude= -I $(Dependencies)/zeromq-4.0.7/include/

BoostLib= -L $(Dependencies)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams
BoostInclude= -I $(Dependencies)/boost_1_66_0/install/include

ToolDAQLib= -L $(Dependencies)/ToolDAQFramework/lib -lServiceDiscovery -lDAQStore -lDAQDataModelBase
ToolDAQInclude= -I $(Dependencies)/ToolDAQFramework/include

ToolFrameworkLib= -L $(Dependencies)/ToolFrameworkCore/lib -lStore -lDataModelBase
ToolFrameworkInclude= -I $(Dependencies)/ToolFrameworkCore/include


all: lib/libDAQInterface.so Win_Mac_translation Example/Example RemoteControl lib/libDAQInterfaceClassDict.so

lib/libDAQInterface.so: src/*.cpp
	g++ -g -O3 -fPIC  -Wpedantic -std=c++11 -shared src/*.cpp -I include -o lib/libDAQInterface.so -lpthread $(BoostInclude) $(BoostLib) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib)

Win_Mac_translation: Win_Mac_translation.cpp
	g++ -O3  -Wpedantic -std=c++11 Win_Mac_translation.cpp -o Win_Mac_translation  -I ./include/ -L lib/ -lDAQInterface -lpthread $(BoostInclude) $(BoostLib) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib)

Example/Example: Example/Example.cpp
	g++ -g -O3  -Wpedantic -std=c++11 Example/Example.cpp -o Example/Example -I ./include/ -L lib/ -lDAQInterface -lpthread $(BoostInclude) $(BoostLib) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib)

lib/libDAQInterfaceClassDict.so: include/DAQInterface.h include/DAQInterfaceLinkdef.h
	rootcling -f src/DAQInterfaceClassDict.cpp -c -p -rmf lib/libDAQInterfaceClassDict.rootmap $^ -I include/ $(ToolFrameworkInclude) $(ToolDAQInclude) $(BoostInclude) $(ZMQInclude)
	g++ -shared -fPIC src/DAQInterfaceClassDict.cpp -o $@ -I `root-config --incdir` -I include/ $(ToolFrameworkInclude) $(ToolDAQInclude) $(BoostInclude) $(ZMQInclude) -L lib -lDAQInterface `root-config --libs`
	cp src/DAQInterfaceClassDict_rdict.pcm lib/

RemoteControl: Dependencies/ToolDAQFramework/src/RemoteControl/RemoteControl.cpp
	g++ -O3  -Wpedantic -std=c++11 Dependencies/ToolDAQFramework/src/RemoteControl/RemoteControl.cpp -o RemoteControl  -I ./include/ -L lib/ -lDAQInterface -lpthread $(BoostInclude) $(BoostLib) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib)

clean:
	rm -f lib/libDAQInterface.so RemoteControl Win_Mac_translation Example/Example
