##################################################
# Build layer
##################################################
FROM debian:latest as build

WORKDIR /build

# Install build dependencies
RUN DEBIAN_FRONTEND=noninteractive \
    apt-get update -y && \
    apt-get install -y \
        git \
        pkg-config \
        g++ \
        gcc \
        cmake \
        uuid-dev \
        liburiparser-dev \
        libexpat1-dev \
        libcurl4-openssl-dev

# Copy, configure and make install
COPY . bmx
ARG BUILD_SHARED_LIBS=ON
ARG LIBMXF_BUILD_ARCHIVE=ON
ARG BMX_BUILD_WITH_LIBCURL=ON
RUN mkdir build && cd build && \
    cmake -G "Unix Makefiles" \
        -DCMAKE_INSTALL_PREFIX=/build/install \
        -DBMX_BUILD_WITH_LIBCURL=$BMX_BUILD_WITH_LIBCURL \
        -DLIBMXF_BUILD_ARCHIVE=$LIBMXF_BUILD_ARCHIVE \
        -DBUILD_SHARED_LIBS=$BUILD_SHARED_LIBS \
        ../bmx && \
    make && make test && make install


##################################################
# Runtime layer
##################################################
FROM debian:latest as runtime

LABEL org.opencontainers.image.source=https://github.com/bbc/bmx
LABEL org.opencontainers.image.description="A useful set of tools for handling MXF and related files"
LABEL org.opencontainers.image.licenses=BSD-3-Clause

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
