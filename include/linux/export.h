/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_EXPORT_H
#define __BACKPORT_EXPORT_H

#include <linux/version.h>
#include_next <linux/export.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
/* Symbol namespaces were introduced in Linux 5.4 with commit
 * 8651ec01daed ("module: add support for symbol namespaces.").
 * Nevertheless, discard symbol namespace before Linux 5.5, since
 * __EXPORT_SYMBOL_NS() in Linux 5.4 inevitably stringifies the
 * namespace argument; whereas Linux 5.5 includes commit c3a6cf19e695
 * ("export: avoid code duplication in include/linux/export.h"), which
 * merges __EXPORT_SYMBOL_NS() into __EXPORT_SYMBOL() and changes
 * the latter macro to take the namespace argument as a string.
 */
#undef EXPORT_SYMBOL_NS_GPL
#define EXPORT_SYMBOL_NS_GPL(sym, ns)   EXPORT_SYMBOL_GPL(sym)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0) && RHEL_RELEASE_CODE < 0x905
/* The macro EXPORT_SYMBOL_NS_GPL() passed the license string "_gpl"
 * to __EXPORT_SYMBOL() before commit ddb5cdbafaaa ("kbuild: generate
 * KSYMTAB entries by modpost"), after which it passes "GPL" instead.
 */
#undef EXPORT_SYMBOL_NS_GPL
#define EXPORT_SYMBOL_NS_GPL(sym, ns)	__EXPORT_SYMBOL(sym, "_gpl", ns)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 13, 0)
/* The macro EXPORT_SYMBOL_NS_GPL() no longer stringifies the namespace
 * argument since commit cdd30ebb1b9f ("module: Convert symbol namespace
 * to string literal"), which must now be done by the caller.
 */
#undef EXPORT_SYMBOL_NS_GPL
#define EXPORT_SYMBOL_NS_GPL(sym, ns)	__EXPORT_SYMBOL(sym, "GPL", ns)
#endif

#endif
