// SPDX-License-Identifier: GPL-2.0

#ifndef _BACKPORT_DEVICE_H_
#define _BACKPORT_DEVICE_H_

#include <linux/version.h>
#include_next <linux/device.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < 0x803
static inline int device_match_of_node(struct device *dev, const void *np)
{
	return dev->of_node == np;
}

/**
 * class_find_device_by_of_node : device iterator for locating a particular device
 * matching the of_node.
 * @class: class type
 * @np: of_node of the device to match.
 */
static inline struct device *
class_find_device_by_of_node(struct class *class, const struct device_node *np)
{
       return class_find_device(class, NULL, np, device_match_of_node);
}
#endif /* < KERNEL_VERSION(5, 4, 0) */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 253) || \
(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0))) && RHEL_RELEASE_CODE < 0x804
/**
 * Simplified version of upstream dev_err_probe.
 */
static inline int dev_err_probe(const struct device *dev, int err, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;

	if (err != -EPROBE_DEFER)
		dev_err(dev, "error %pe: %pV", ERR_PTR(err), &vaf);
	else
		dev_dbg(dev, "error %pe: %pV", ERR_PTR(err), &vaf);

	va_end(args);

	return err;
}
#endif

#endif
