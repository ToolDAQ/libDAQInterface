Dependencies=./Dependencies

ZMQLib= -L $(Dependencies)/zeromq-4.0.7/lib -lzmq
ZMQInclude= -I $(Dependencies)/zeromq-4.0.7/include/

BoostLib= -L $(Dependencies)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams
BoostInclude= -I $(Dependencies)/boost_1_66_0/install/include

ToolDAQLib= -L $(Dependencies)/ToolDAQFramework/lib   -lDAQDataModelBase -lServiceDiscovery -lDAQStore
ToolDAQInclude= -I $(Dependencies)/ToolDAQFramework/include

ToolFrameworkLib= -L $(Dependencies)/ToolFrameworkCore/lib -lStore -lDataModelBase
ToolFrameworkInclude= -I $(Dependencies)/ToolFrameworkCore/include

RootInclude= -I`root-config --incdir`
RootFlags=`root-config --cflags`
RootLib= `root-config --libs`

sources= $(filter-out  %DAQInterfaceClassDict.cpp, $(wildcard src/*.cpp))

all: lib/libDAQInterface.so Win_Mac_translation Example/Example RemoteControl

lib/libDAQInterface.so: $(sources)
	g++ -O3 -fPIC  -Wpedantic -std=c++11 -shared src/DAQInterface.cpp -I include -o lib/libDAQInterface.so -lpthread  $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(BoostLib)

Win_Mac_translation: Win_Mac_translation.cpp lib/libDAQInterface.so
	g++ -O3  -Wpedantic -std=c++11 Win_Mac_translation.cpp -o Win_Mac_translation  -I ./include/ -L lib/ -lDAQInterface -lpthread  $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(BoostLib) $(ToolDAQLib)  $(BoostLib)

# this is the default example showing the majority of features
Example/Example: Example/Example.cpp lib/libDAQInterface.so
	g++ -O3  -Wpedantic -std=c++14 $^ -o $@ -I ./include/ -L lib/ -lDAQInterface -lpthread $(ToolDAQInclude) $(ToolDAQLib) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(BoostLib) $(ToolDAQLib) 

# this is required ONLY to demonstrate the use of storing and retreiving ROOT plots in the database
Example/Example_root: Example/Example_root.cpp lib/libDAQInterface.so
	g++ -O3  -Wpedantic -std=c++14 $(RootFlags) $^ -o $@ -I ./include/ -L lib/ -lDAQInterface -lpthread $(ToolDAQInclude) $(ToolDAQLib) $(RootLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolDAQLib) $(ToolFrameworkLib) $(BoostInclude) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(BoostLib) $(ToolDAQLib)

# this is required ONLY if you want to run the python example, or use the libDAQInterface in python
lib/libDAQInterfaceClassDict.so: include/DAQInterface.h include/DAQInterfaceLinkdef.h
	rootcling -f src/DAQInterfaceClassDict.cpp -c -p -rmf lib/libDAQInterfaceClassDict.rootmap $^ -I ./include/ $(ToolFrameworkInclude) $(ToolDAQInclude) $(BoostInclude) $(ZMQInclude)
	g++ -shared -fPIC src/DAQInterfaceClassDict.cpp -o $@ -I ./ -I ./include/ $(ToolFrameworkInclude) $(ToolDAQInclude) $(BoostInclude) $(ZMQInclude) $(RootInclude) -L lib -lDAQInterface $(RootLib)
	cp src/DAQInterfaceClassDict_rdict.pcm lib/

RemoteControl: Dependencies/ToolDAQFramework/src/RemoteControl/RemoteControl.cpp lib/libDAQInterface.so
	g++ -O3  -Wpedantic -std=c++11 Dependencies/ToolDAQFramework/src/RemoteControl/RemoteControl.cpp -o RemoteControl  -I ./include/ -L lib/ -lDAQInterface -lpthread $(BoostInclude) $(BoostLib) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(ToolDAQLib) $(BoostLib)

clean:
	rm -f lib/libDAQInterface.so RemoteControl Win_Mac_translation Example/Example Example/Example_root
