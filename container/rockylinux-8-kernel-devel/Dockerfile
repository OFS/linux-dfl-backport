ARG tag=8
# Pull non-official image to retrieve archived versions that were
# removed from the official library, such as rockylinux:8.4.
# https://forums.rockylinux.org/t/rockylinux-8-4-not-in-official-rockylinux-docker-images-tag-list/5248
FROM rockylinux/rockylinux:${tag}
# https://docs.docker.com/engine/reference/builder/#understand-how-arg-and-from-interact
ARG tag
# The PowerTools repository is needed for dwarves, which is a
# build dependency of the kernel makefile target rpm-pkg.
# The EPEL repository is needed for dkms.
#
# When ${tag} is explicitly given as a minor version, e.g., 8.6,
# install packages from the vault repositories instead of the default
# mirrors. Otherwise, packages will be upgraded or installed from the
# latest minor release, e.g., 8.6 is upgraded to 8.9 or 8.10, which
# we want to specifically avoid, to build against older kernels.
RUN sed -i '/^enabled=/s#0#1#' /etc/yum.repos.d/Rocky-PowerTools.repo \
    && grep '^enabled=1$' /etc/yum.repos.d/Rocky-PowerTools.repo \
    && ([[ "${tag}" != *.* ]] || sed -i -e '/^mirrorlist/s,^,#,' \
        -e "s,^#\\?\\(baseurl=http://dl.rockylinux.org\\)/\$contentdir/\$releasever/,\\1/vault/rocky/${tag}/," \
        /etc/yum.repos.d/Rocky-*.repo) \
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
