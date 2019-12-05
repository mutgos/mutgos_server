FROM ubuntu:18.04

# Get g++ for compiling and git and wget for retrieving things
# and install sqlite3 from a package
RUN apt-get update \
    && apt-get install -y git \
                          cmake \
                          wget \
                          g++ \
                          unzip \
                          sqlite3 \
                          libssl-dev \
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
ADD ./third_party /home/mutgos/third_party
ADD ./CMakeFiles /home/mutgos/CMakeFiles
ADD ./CMakeLists.txt /home/mutgos/CMakeLists.txt
RUN cd /home/mutgos \
    && cmake . \
    && make

# Make sure boost libraries can be found.
ENV LD_LIBRARY_PATH /usr/local/lib

# Handling config data, etc.
# TODO(hyena): Come up with a solution that maps ./data
# from the host so that the user can backup, tweak,
# etc.
ADD ./data /home/mutgos/data

# Copy the sample config file but not if the user's written one.
RUN if [ ! -f /home/mutgos/data/mutgos.conf ]; then \
        cp /home/mutgos/data/mutgos.conf.sample /home/mutgos/data/mutgos.conf \
    ; fi

# Generate a new SSL key and cert if the user doesn't have either.
RUN if [ ! -f /home/mutgos/data/key.pem ] || [ ! -f /home/mutgos/data/cert.pem ]; then \
        cd /home/mutgos/data \
        && openssl req -x509 -subj '/CN=localhost' -newkey rsa:4096 -nodes -keyout key.pem -out cert.pem \
    ; fi

# Build the db and put it in the right place.
RUN if [ ! -f /home/mutgos/data/mutgos.db ]; then \
        cd /home/mutgos/src/exe/read_dump \
        && ./readdump --configfile /home/mutgos/data/mutgos.conf --dumpfile /home/mutgos/data/prototype_db.dump \
    ; fi


EXPOSE 7072 7073
WORKDIR /home/mutgos/src/exe/mutgos_server
ENTRYPOINT ["./mutgos_server", "--configfile", "../../../data/mutgos.conf"]
