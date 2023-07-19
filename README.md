# Stand-alone DAQInterface library

This code provides an interface library to the SQL databases, monitoring and slow control systems. 

# Installation

To install the required dependencies run:

      ./GetDepenencies.sh

This will install ZMQ, Boost, ToolFramework and ToolDAQFramework in `./Dependencies`. 
Or if you already have them you can change the Makefile paths to point to your existing local copies. 

After installing dependencies run:

      make

to compile. This will produce:

    1) The DAQ Interface shared object library (lib/libDAQInterface.so) that can be used with your own stand alone code
    2) An Example application (Example/Example) demonstrating the interface usage (source code: Example/Example.cpp)
    3) A command line remote control application (RemoteControl) for sending slow control commands
    4) A Windows and MacOS interface translation application (Win_Mac_translation)


# Usage/Execution

To use the DAQ Interface library in your own standalone program include the header in your code 
  
    #include <DAQInterface.h>

and compile with 

    g++ yourprogram.cpp -o yourprogram -I <path to repo>/include -L <path to repo>/lib -lDAQInterface

For a list of functions provided by the interface see `include/DAQInterface.h`, and for example usage refer to `Example/Example.cpp`

Before executing, configure your environment by calling:

    source Setup.sh


# Example

To run the example:

    source Setup.sh
    ./Example

**Note: For Windows or MacOS Users running the web server in a Docker container, and the client application on the same machine directly in the host OS.**
Due to networking limitations with Docker you will need to run the `Win_Mac_translation` application in the background on the host OS, before starting the standalone client application:

    ./Win_Mac_translation &

# Using the DAQInterface library in Python

With [cppyy](https://github.com/wlav/cppyy) it's possible to import the `DAQInterface` class into python with virtually seamless integration. An example python script is provided in `Example/Example.py`, which closely mirrors the c++ example to demonstrate the equivalence in use from the two languages.

As cppyy is part of the standard python bindings used by ROOT it comes with all recent ROOT versions, so the simplest way to install cppyy is to install ROOT.
After following the normal installation steps, python support can therefore be added by invoking:

    ./GetDependencies.sh --python

which will install a recent ROOT version (6.24 or 6.28, depending on your compiler) into `./Dependencies`, with the necessary components enabled.
From there, you can run

      python3 Example/Example.py

to run the example python client.
