// SPDX-License-Identifier: GPL-2.0

#ifndef _BACKPORT_DEVICE_H_
#define _BACKPORT_DEVICE_H_

#include <linux/version.h>
#include_next <linux/device.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
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

#endif
