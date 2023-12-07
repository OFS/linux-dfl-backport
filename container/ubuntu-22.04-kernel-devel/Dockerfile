ARG tag=22.04
FROM ubuntu:${tag}
RUN apt-get -qq update \
    && apt-get -qq upgrade --no-install-recommends \
    bc \
    bison \
    build-essential \
    ca-certificates \
    cpio \
    debhelper \
    devscripts \
    dkms \
    flex \
    git \
    kmod \
    libelf-dev \
    libncurses-dev \
    libssl-dev \
    linux-image-generic \
    linux-headers-generic \
    openssl \
    pkg-config \
    python3 \
    rsync \
    wget \
    zstd \
    && apt-get clean
