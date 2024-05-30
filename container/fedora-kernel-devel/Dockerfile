ARG tag=rawhide
FROM fedora:${tag}
# https://docs.docker.com/engine/reference/builder/#understand-how-arg-and-from-interact
ARG tag
RUN dnf -y upgrade \
    && dnf -y install \
    bc \
    bison \
    ca-certificates \
    cpio \
    dkms \
    dwarves \
    elfutils-libelf-devel \
    flex \
    gcc \
    git \
    kernel \
    kernel-devel \
    kernel-headers \
    make \
    ncurses-devel \
    openssl-devel \
    perl \
    pkg-config \
    python3 \
    rpm-build \
    rsync \
    && dnf -y clean all
