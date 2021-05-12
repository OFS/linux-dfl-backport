Name:               linux-dfl-backport
Version:            %{getenv:BACKPORT_VERSION}
Release:            1
Summary:            Backported kernel modules for in-development linux fpga dfl modules
License:            GPLv2
Group:              System/Kernel and hardware
URL:                https://github.com/OPAE/linux-dfl-backport/
BuildArch:          noarch
Requires:           dkms, (kernel-devel if kernel), (kernel-rt-devel if kernel-rt)

%define _dstdir %{_usrsrc}/linux-dfl-backport-%{version}-%{release}
%define _pkgdir %{buildroot}%{_dstdir}
%define _dracut %{_prefix}/lib/dracut/dracut.conf.d/90-linux-dfl-backport.conf

%description
Device Feature List (dfl) is used by FPGAs to communicate features
supported by a given loaded firmware/bitstream. Since dfl support in the
Linux kernel has yet to materialize, this package contains a backported
version of the current dfl support.

%install
install -d %{_pkgdir}
cp -a build/dkms/generate-dkms-conf.sh Makefile drivers include %{_pkgdir}
sed -E 's/PACKAGE_VERSION=".+"/PACKAGE_VERSION="%{version}-%{release}"/' build/dkms/dkms.conf > %{_pkgdir}/dkms.conf
install -d $(dirname %{buildroot}%{_dracut})
echo 'omit_drivers+="%_modules"' > %{buildroot}%{_dracut}

%post
dkms add %{name}/%{version}-%{release} --rpm_safe_upgrade --no-initrd
dkms install %{name}/%{version}-%{release} --rpm_safe_upgrade --no-initrd

%preun
make -C %{_dstdir} rmmod
dkms remove %{name}/%{version}-%{release} --rpm_safe_upgrade --no-initrd --all

%postun
rmdir %{_dstdir}

%files
%defattr(-,root,root)
%{_dstdir}/*
%{_dracut}
