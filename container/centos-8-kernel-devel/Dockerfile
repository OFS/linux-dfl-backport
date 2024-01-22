ARG tag=8.3.2011
FROM centos:${tag}
# https://docs.docker.com/engine/reference/builder/#understand-how-arg-and-from-interact
ARG tag
# On CentOS 8, the PowerTools repository is needed for dwarves, which
# is a build dependency of the kernel makefile target rpm-pkg.
# The EPEL repository is needed for dkms.
#
# Install packages from CentOS Vault Mirror since mirrorlist is empty.
RUN sed -i '/^enabled=/s#0#1#' /etc/yum.repos.d/CentOS-*PowerTools.repo \
    && sed -i -e '/^mirrorlist/s,^,#,' \
        -e "s,^#\\?\\(baseurl=http://\\)mirror.centos.org/\$contentdir/\$releasever/,\\1/vault.centos.org/${tag}/," \
        /etc/yum.repos.d/CentOS-*.repo \
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
