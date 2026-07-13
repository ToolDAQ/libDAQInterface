FROM tooldaq/core
   
RUN cd /opt \
    && cd ToolFrameworkCore \
    && git pull \
    && make clean \
    && make -j `nproc --all` \
    && cd ../ToolDAQFramework \
    && git pull \
    && make clean \
    && make -j `nproc --all` \
    && cd ../ \
    && git clone https://github.com/ToolDAQ/libDAQInterface.git \
    && cd libDAQInterface \
    && ln -s /opt ./Dependencies \
    && chmod a+x  ./Setup.sh \
    && source ./Setup.sh \
    && make clean \
    && make -j `nproc --all` \
    && chmod -R a+rw /opt/libDAQInterface
