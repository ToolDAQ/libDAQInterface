#!/bin/bash

threads=`nproc --all`

APPDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p Dependencies
cd Dependencies
DEPDIR=`pwd`

# sanitize arguments list
argstring=""
PATTERN='\-\-([^ =]+)[ =]?([01])?'
LOOPCOUNT=0
flagstring=$*
while [[ $flagstring =~ ${PATTERN} ]]; do
	KEY="${BASH_REMATCH[1]}"
	VALUE="${BASH_REMATCH[2]}"
	if [ -z "${VALUE}" ]; then
		VALUE=1
	fi
	argstring+="${KEY}=${VALUE} "
	# remove this option from input flagstring
	flagstring="${flagstring/${BASH_REMATCH[0]}/}"
	let LOOPCOUNT=$LOOPCOUNT+1
	if [ $LOOPCOUNT -gt 10 ]; then
		break;
	fi
done

# array of things to install
declare -A flags

if [ -z ${argstring} ]; then
	# if no arguments given, start with default selections
	flags["zmq"]="on"
	flags["boost"]="on"
	flags["toolframework"]="on"
	flags["tooldaq"]="on"
	flags["python"]="off"
	
	# if interactive terminal and dialog is installed, present a check-box of options
	which dialog &>/dev/null
	if [ $? -eq 0 ] && [ "${-#*i}" == "$-" ]; then
		alias dialog='dialog --backtitle "Postgres Setup" --aspect 100 --cr-wrap'
		
		selectionstring=$(dialog --backtitle "Dependency Setup" --aspect 100 --cr-wrap \
			       --checklist "Please select dependencies to install" 20 80 9 \
		toolframework "Install ToolFramework" ${flags['toolframework']} \
		tooldaq "Install ToolDAQFramework" ${flags['tooldaq']} \
		boost "Install Boost" ${flags['boost']} \
		zmq "Install ZMQ" ${flags['zmq']} \
		python "Install Python Support (note: will install ROOT if not found)" ${flags['python']} \
		2>&1 1>/dev/tty)
		
		# split space-delimited string of selected options into an array
		read -r -a selections <<< "${selectionstring}"
		
		# update our flags
		for flag in ${!flags[*]}; do
			if [[ " ${selections[*]} " =~ " ${flag} " ]]; then
				flags[$flag]="on"
			else
				flags[$flag]="off"
			fi
		done
	fi
	
	# TODO offer a non-dialog alternative in an interactive terminal?
	
else
	# if user specified elements to install, start with nothing...
	flags["zmq"]="off"
	flags["boost"]="off"
	flags["toolframework"]="off"
	flags["tooldaq"]="off"
	flags["python"]="off"
	
	# ... and parse arguments for user selections
	for flag in ${argstring[*]}; do
		echo "setting option ${flag}"
		case $flag in
			zmq*|boost*|toolframework*|tooldaq*|python*)
				feature=${flag%=*}
				feature=${feature##--}
				setting=${flag##*=}
				if [ $setting -eq 1 ]; then
					flags["${feature}"]="on"
				else
					flags["${feature}"]="off"
				fi
				;;
			*)
				echo "unknown flag ${flag}"
				exit 1
				;;
		esac
	done
fi

echo "selected dependencies: ${flags[*]}"

# enforce dependencies of dependencies?
oldflags="${flags[*]}"
if [ "${flags['tooldaq']}" == "on" ] && [ "${flags['toolframework']}" == "off" ]; then
	echo "tooldaq requires toolframework, adding to list of installed elements..."
	flags['toolframework']="on"
fi
if [ "${flags['tooldaq']}" == "on" ] && [ "${flags['boost']}" == "off" ]; then
	echo "tooldaq requires boost, adding to list of installed elements..."
	flags['boost']="on"
fi
if [ "${flags['tooldaq']}" == "on" ] && [ "${flags['zmq']}" == "off" ]; then
	echo "tooldaq requires zmq, adding to list of installed elements..."
	flags['zmq']="on"
fi

# prompt for confirmation if interactive and we've modified user selections
if [ "${flags[*]}" != "${oldflags}" ] && [ "${-#*i}" == "$-" ]; then
	echo "The following elements will be installed: ";
	for element in ${!flags[*]}; do
		if [ "${flags[${element}]}" == "on" ]; then
			echo -e "\t$element "
		fi
	done
	echo "Continue?"
	select result in Continue Abort; do
		if [ "$result" == "Continue" ] || [ "$result" == "Abort" ]; then
			if [ "$result" == "Abort" ]; then
				echo "terminating";
				exit 1;
			else
				echo "continuing..."
				break;
			fi
		else
			echo "please enter 1 or 2";
		fi
	done
fi

# proceed with installation
if [ "${flags['zmq']}" == "on" ]; then
	cd ${DEPDIR}
	git clone https://github.com/ToolDAQ/zeromq-4.0.7.git
	cd zeromq-4.0.7
	
	./configure --prefix=`pwd`
	make -j $threads
	make install
	export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi

if [ "${flags['boost']}" == "on" ]; then
	cd ${DEPDIR}
	git clone https://github.com/ToolDAQ/boost_1_66_0.git
	cd boost_1_66_0
	
	rm -rf INSTALL
	mkdir install
	./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null
	./b2 install iostreams -j $threads
	
	export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH
fi

if [ "${flags['toolframework']}" == "on" ]; then
	cd ${DEPDIR}
	git clone https://github.com/ToolFramework/ToolFrameworkCore.git
	cd ToolFrameworkCore
	
	make clean
	make -j $threads
	export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi

if [ "${flags['tooldaq']}" == "on" ]; then
	cd ${DEPDIR}
	git clone https://github.com/ToolDAQ/ToolDAQFramework.git
	cd ToolDAQFramework
	
	make clean
	make -j $threads
	export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi

if [ "${flags['python']}" == "on" ]; then
	cd ${DEPDIR}
	${APPDIR}/scripts/Python/Import.sh $APPDIR /opt
fi

cd ${APPDIR}
make

