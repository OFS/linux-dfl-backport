FROM rockylinux:8
# The PowerTools repository is needed for dwarves, which is a
# build dependency of the kernel makefile target rpm-pkg.
# The EPEL repository is needed for dkms.
RUN sed -i '/^enabled=/s#0#1#' /etc/yum.repos.d/Rocky-PowerTools.repo \
    && grep '^enabled=1$' /etc/yum.repos.d/Rocky-PowerTools.repo \
    && yum -y upgrade \
    && yum -y install epel-release \
    && yum -y install \
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
    && yum -y clean all
