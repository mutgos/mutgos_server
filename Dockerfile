FROM ubuntu:18.04

### TODO: This is temporarily broken due to the new configfile setup.  It will compile but not run.

# Get g++ for compiling and git and wget for retrieving things
# and install sqlite3 from a package
RUN apt-get update \
    && apt-get install -y git \
                          cmake \
                          wget \
                          g++ \
                          unzip \
                          sqlite3 \
                          libsqlite3-dev

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
    && cat CMakeLists.txt \
    && cmake -DCMAKE_CXX_FLAGS=-Wno-implicit-fallthrough . \
    && make install \
    && cd /home \
    && rm -rf rapidjson

# Install angelscript 2.33
RUN cd /home \
    && wget https://www.angelcode.com/angelscript/sdk/files/angelscript_2.33.0.zip \
    && unzip angelscript_2.33.0.zip \
    && rm angelscript_2.33.0.zip \
    && cd sdk/angelscript/projects/gnuc \
    && make PREFIX=/usr/local clean \
    && CXXFLAGS="-g" make PREFIX=/usr/local \
    && make PREFIX=/usr/local install \
    && cd /home \
    && rm -rf sdk

# Build mutgos
# Copy in only those files that affect the build
# so tweaking a readme doesn't confuse the Dockerfile
ADD ./src /home/mutgos/src
ADD ./CMakeFiles /home/mutgos/CMakeFiles
ADD ./CMakeLists.txt /home/mutgos/CMakeLists.txt
RUN cd /home/mutgos \
    && cmake . \
    && make

# Make sure boost libraries can be found.
ENV LD_LIBRARY_PATH /usr/local/lib

# Build the db and put it in the right place.
# TODO(hyena): Come up with a solution that maps ./data
# from the host so that the user can backup, tweak,
# etc.
ADD ./data /home/mutgos/data
RUN cd /home/mutgos/src/exe/read_dump \
    && ./readdump /home/mutgos/data/prototype_db.dump \
    && cp ./mutgos.db ../mutgos_server


EXPOSE 7072
WORKDIR /home/mutgos/src/exe/mutgos_server
ENTRYPOINT ["./mutgos_server"]
