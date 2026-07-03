#!/bin/bash

Dependencies=`pwd`/Dependencies
export PS1='${debian_chroot:+($debian_chroot)}\[\033[01;34m\]\u@\h\[\033[00m\]:\[\033[00;36m\]\w\[\033[00m\]\$ '

export LD_LIBRARY_PATH=`pwd`/lib:${Dependencies}/zeromq-4.0.7/lib:${Dependencies}/boost_1_66_0/install/lib:${Dependencies}/ToolDAQFramework/lib:${Dependencies}/ToolFrameworkCore/lib:$LD_LIBRARY_PATH
