#!/bin/bash

Dependencies=`pwd`/Dependencies

export LD_LIBRARY_PATH=`pwd`/lib:${Dependencies}/zeromq-4.0.7/lib:${Dependencies}/boost_1_66_0/install/lib:${Dependencies}/ToolDAQFramework/lib:${Dependencies}/ToolFrameworkCore/lib:$LD_LIBRARY_PATH
