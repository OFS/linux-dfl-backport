Name:               linux-dfl-backport
Summary:            Backported fpga drivers from linux-dfl
License:            GPLv2
Group:              System/Kernel and hardware
URL:                https://github.com/OPAE/linux-dfl-backport/
BuildArch:          noarch
Requires:           dkms >= 3.0.0
Requires:           (kernel-devel if kernel), (kernel-rt-devel if kernel-rt)

%define os_branch   rhel8
%define lts_tag     v5.15.6
Release:            1
Version:            %{os_branch}_%{lts_tag}

%define _dstdir %{_usrsrc}/linux-dfl-backport-%{version}-%{release}
%define _pkgdir %{buildroot}%{_dstdir}
%define _dracut %{_prefix}/lib/dracut/dracut.conf.d/90-linux-dfl-backport.conf

%description
A backport of DFL FPGA drivers from the current LTS branch of
https://github.com/OPAE/linux-dfl-backport/.

%install
install -d %{_pkgdir}
cp -a build/dkms/generate-dkms-conf.sh Makefile drivers include %{_pkgdir}
sed -E 's/PACKAGE_VERSION=".+"/PACKAGE_VERSION="%{version}-%{release}"/' build/dkms/dkms.conf > %{_pkgdir}/dkms.conf
install -d $(dirname %{buildroot}%{_dracut})
echo 'omit_drivers+="%_modules"' > %{buildroot}%{_dracut}

%post
dkms add %{name}/%{version}-%{release} --rpm_safe_upgrade
dkms install %{name}/%{version}-%{release} --rpm_safe_upgrade
modprobe -a dfl_pci

%preun
make -C %{_dstdir} rmmod
dkms remove %{name}/%{version}-%{release} --rpm_safe_upgrade --all

%postun
rmdir %{_dstdir}

%files
%defattr(-,root,root)
%{_dstdir}/*
%{_dracut}
