Dependencies=./Dependencies

CXXFLAGS=-g -fmax-errors=3 -std=c++20 -Werror=array-bounds -Werror=return-type -Wpedantic

ifeq ($(MAKECMDGOALS),debug)
CXXFLAGS+= -O0 -lSegFault -rdynamic -DDEBUG
else
CXXFLAGS+= -O3
endif

ZMQLib= -L $(Dependencies)/zeromq-4.0.7/lib -lzmq
ZMQInclude= -I $(Dependencies)/zeromq-4.0.7/include/

BoostLib= -L $(Dependencies)/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams
BoostInclude= -I $(Dependencies)/boost_1_66_0/install/include

ToolDAQLib= -L $(Dependencies)/ToolDAQFramework/lib   -lServiceDiscovery -lDAQDataModelBase -lDAQStore
ToolDAQInclude= -I $(Dependencies)/ToolDAQFramework/include

ToolFrameworkLib= -L $(Dependencies)/ToolFrameworkCore/lib -lStore -lDataModelBase
ToolFrameworkInclude= -I $(Dependencies)/ToolFrameworkCore/include

RootInclude= -I`root-config --incdir`
RootFlags=`root-config --cflags`
RootLib= `root-config --libs`

sources= $(filter-out  %DAQInterfaceClassDict.cpp, $(wildcard src/*.cpp) $(wildcard include/*.h))

.phony: python

debug: all

all: lib/libDAQInterface.so Win_Mac_translation Example/Example Example/Test RemoteControl

lib/libDAQInterface.so: $(sources)
	g++ $(CXXFLAGS) -fPIC -shared src/DAQInterface.cpp -I include -o lib/libDAQInterface.so -lpthread  $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(BoostLib)

Win_Mac_translation: Win_Mac_translation.cpp lib/libDAQInterface.so
	g++ $(CXXFLAGS) Win_Mac_translation.cpp -o Win_Mac_translation  -I ./include/ -L lib/ -lDAQInterface -lpthread  $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(BoostLib) $(ToolDAQLib)  $(BoostLib)

# this is the default example showing the majority of features
Example/Example: Example/Example.cpp lib/libDAQInterface.so
	g++ $(CXXFLAGS) $^ -o $@ -I ./include/ -L lib/ -lDAQInterface -lpthread $(ToolDAQInclude) $(ToolDAQLib) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(BoostLib) $(ToolDAQLib)

# this is required ONLY to demonstrate the use of storing and retreiving ROOT plots in the database
Example/Example_root: Example/Example_root.cpp lib/libDAQInterface.so
	g++ $(CXXFLAGS) $(RootFlags) $^ -o $@ -I ./include/ -L lib/ -lDAQInterface -lpthread $(ToolDAQInclude) $(ToolFrameworkInclude) $(BoostInclude) $(ZMQInclude) $(ToolFrameworkLib) $(ToolDAQLib) $(ZMQLib) $(BoostLib) $(RootLib)

# this is required ONLY if you want to run the python example, or use the libDAQInterface in python
python: lib/libDAQInterface.so lib/libDAQInterfaceClassDict.so

# functionality testing
Example/Test: Example/Test.cpp lib/libDAQInterface.so
	g++ $(CXXFLAGS) $^ -o $@ -I ./include/ -L lib/ -lDAQInterface -lpthread $(ToolDAQInclude) $(ToolDAQLib) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(BoostLib) $(ToolDAQLib)

# throttle testing
Example/Test2: Example/Test2.cpp lib/libDAQInterface.so
	g++ $(CXXFLAGS) $^ -o $@ -I ./include/ -L lib/ -lDAQInterface -lpthread $(ToolDAQInclude) $(ToolDAQLib) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(BoostInclude) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(BoostLib) $(ToolDAQLib)

lib/libDAQInterfaceClassDict.so: include/DAQInterface.h include/DAQInterfaceLinkdef.h
	rootcling -f src/DAQInterfaceClassDict.cpp -c -p -rmf lib/libDAQInterfaceClassDict.rootmap $^ -I ./include/ $(ToolFrameworkInclude) $(ToolDAQInclude) $(BoostInclude) $(ZMQInclude)
	g++ -shared $(CXXFLAGS) -fPIC src/DAQInterfaceClassDict.cpp -o $@ -I ./ -I ./include/ $(ToolFrameworkInclude) $(ToolDAQInclude) $(BoostInclude) $(ZMQInclude) $(RootInclude) -L lib -lDAQInterface $(RootLib)
	cp src/DAQInterfaceClassDict_rdict.pcm lib/
# end python requirements

RemoteControl: $(Dependencies)/ToolDAQFramework/src/RemoteControl/RemoteControl.cpp lib/libDAQInterface.so
	g++ $(CXXFLAGS) $(Dependencies)/ToolDAQFramework/src/RemoteControl/RemoteControl.cpp -o RemoteControl  -I ./include/ -L lib/ -lDAQInterface -lpthread $(BoostInclude) $(BoostLib) $(ZMQInclude) $(ZMQLib) $(ToolDAQLib) $(ToolDAQInclude) $(ToolFrameworkInclude) $(ToolFrameworkLib) $(ToolDAQLib) $(BoostLib)

clean:
	rm -f lib/libDAQInterface.so \
	RemoteControl \
	Win_Mac_translation \
	Example/Example \
	Example/Example_root \
	lib/DAQInterfaceClassDict_rdict.pcm \
	lib/libDAQInterfaceClassDict.rootmap \
	lib/libDAQInterfaceClassDict.so

