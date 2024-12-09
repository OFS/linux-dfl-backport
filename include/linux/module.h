/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_MODULE_H
#define __BACKPORT_MODULE_H

#include <linux/version.h>
#include_next <linux/module.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
/* Symbol namespaces were introduced in Linux 5.4 with commit
 * 8651ec01daed ("module: add support for symbol namespaces.").
 * Nevertheless, discard symbol namespace before Linux 5.5; see
 * backported EXPORT_SYMBOL_NS_GPL() in export.h for the reason.
 */
#undef MODULE_IMPORT_NS
#define MODULE_IMPORT_NS(ns)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 13, 0)
/* The macro MODULE_IMPORT_NS() no longer stringifies its argument
 * since commit cdd30ebb1b9f ("module: Convert symbol namespace to
 * string literal"), which must now be done by the caller.
 */
#undef MODULE_IMPORT_NS
#define MODULE_IMPORT_NS(ns)	MODULE_INFO(import_ns, ns)
#endif

#endif
