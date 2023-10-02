/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_UUID_H
#define __BACKPORT_UUID_H

#include <linux/version.h>
#include_next <linux/uuid.h>

static inline
bool guid_parse_and_compare(const char *string, const guid_t *guid)
{
	guid_t guid_input;
	if (guid_parse(string, &guid_input))
		return false;
	return guid_equal(&guid_input, guid);
}

static inline
bool uuid_parse_and_compare(const char *string, const uuid_t *uuid)
{
	uuid_t uuid_input;
	if (uuid_parse(string, &uuid_input))
		return false;
	return uuid_equal(&uuid_input, uuid);
}

#endif
