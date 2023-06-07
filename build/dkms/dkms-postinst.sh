#!/bin/sh
# Copyright(c) 2022, Intel Corporation
depmod -A
if egrep --quiet '^dfl_pci ' /proc/modules; then
	echo
	echo   '*** Another version of the driver is already loaded. Please'
	echo   '*** reboot your machine to activate the newly installed driver.'
	echo
else
	modprobe dfl_pci
fi
