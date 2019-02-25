FROM ubuntu:18.04

# Get g++ for compiling and git and wget for retrieving things
# and install sqlite3 from a package
RUN apt-get update \
    && apt-get install -y git \
                          cmake \
                          wget \
                          g++ \
                          unzip \
                          sqlite3

# Install Boost
RUN cd /home && wget http://downloads.sourceforge.net/project/boost/boost/1.67.0/boost_1_67_0.tar.gz \
    && tar xfz boost_1_67_0.tar.gz \
    && rm boost_1_67_0.tar.gz
RUN cd /home/boost_1_67_0 \
    && ./bootstrap.sh --prefix=/usr/local \
    && ./b2 install \
    && cd /home \
    && rm -rf boost_1_67_0

# Install rapidjson 1.1.0
RUN cd /home \
    && git clone --branch v1.1.0 https://github.com/Tencent/rapidjson.git \
    && cd rapidjson \
    && cmake . \
    && make install \
    && cd /home \
    && rm -rf rapidjson

# Install angelscript 2.32
RUN cd /home \
    && wget https://www.angelcode.com/angelscript/sdk/files/angelscript_2.32.0.zip \
    && unzip angelscript_2.32.0.zip \
    && rm angelscript_2.32.0.zip \
    && cd sdk/angelscript/projects/cmake \
    && cmake . \
    && make install \
    && cd /home \
    && rm -rf sdk

RUN 
