##################################################
# Build layer
##################################################
FROM debian:latest as build

ARG LIBMXF_GIT=https://git.code.sf.net/p/bmxlib/libmxf
ARG LIBMXFPP_GIT=https://git.code.sf.net/p/bmxlib/libmxfpp

WORKDIR /build

# Install build dependencies
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update -y && \
    apt-get install -y \
        git \
        pkg-config \
        automake \
        libtool \
        g++ \
        gcc \
        make \
        autoconf \
        uuid-dev \
        liburiparser-dev \
        libexpat1-dev \
        libcurl4-openssl-dev

# Clone, configure and make install libMXF, libMXF++ and bmx

RUN git clone $LIBMXF_GIT libMXF && \
    cd libMXF && \
    ./autogen.sh && \
    ./configure PKG_CONFIG_PATH=/build/install/lib/pkgconfig --prefix /build/install --disable-static && \
    make check && \
    make install

RUN git clone $LIBMXFPP_GIT libMXFpp && \
    cd libMXFpp && \
    ./autogen.sh && \
    ./configure PKG_CONFIG_PATH=/build/install/lib/pkgconfig --prefix /build/install --disable-static --disable-examples && \
    make check && \
    make install

COPY . bmx/
RUN cd bmx && \
    ./autogen.sh && \
    ./configure PKG_CONFIG_PATH=/build/install/lib/pkgconfig --prefix /build/install --disable-static && \
    make check && \
    make install


##################################################
# Runtime layer
##################################################
FROM debian:latest as runtime

# Install runtime dependencies
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update -y && \
    apt-get install -y \
        uuid-dev \
        liburiparser-dev \
        libexpat1-dev \
        libcurl4-openssl-dev

# Copy build artefacts and update the runtime linker cache with those artefacts
COPY --from=build /build/install/ /usr/local/
RUN /sbin/ldconfig

COPY docker/tool_list.txt /

CMD [ "cat", "/tool_list.txt" ]
