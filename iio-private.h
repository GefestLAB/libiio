/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * libiio - Library for interfacing industrial I/O (IIO) devices
 *
 * Copyright (C) 2014 Analog Devices, Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
 */

#ifndef __IIO_PRIVATE_H__
#define __IIO_PRIVATE_H__

/* Include public interface */
#include "iio.h"
#include "iio-backend.h"
#include "iio-config.h"
#include "iio-debug.h"

#include <stdbool.h>

#define MAX_CHN_ID     NAME_MAX  /* encoded in the sysfs filename */
#define MAX_CHN_NAME   NAME_MAX  /* encoded in the sysfs filename */
#define MAX_DEV_ID     NAME_MAX  /* encoded in the sysfs filename */
#define MAX_DEV_NAME   NAME_MAX  /* encoded in the sysfs filename */
#define MAX_CTX_NAME   NAME_MAX  /* nominally "xml" */
#define MAX_CTX_DESC   NAME_MAX  /* nominally "linux ..." */
#define MAX_ATTR_NAME  NAME_MAX  /* encoded in the sysfs filename */
#define MAX_ATTR_VALUE (8 * PAGESIZE)  /* 8x Linux page size, could be anything */

#ifdef _MSC_BUILD
/* Windows only runs on little-endian systems */
#define is_little_endian() true
#else
#define is_little_endian() (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#endif

#define BIT(x) (1 << (x))
#define BIT_MASK(bit) BIT((bit) % 32)
#define BIT_WORD(bit) ((bit) / 32)

/* ntohl/htonl are a nightmare to use in cross-platform applications,
 * since they are defined in different headers on different platforms.
 * iio_be32toh/iio_htobe32 are just clones of ntohl/htonl. */
static inline uint32_t iio_be32toh(uint32_t word)
{
	if (!is_little_endian())
		return word;

#ifdef __GNUC__
	return __builtin_bswap32(word);
#else
	return ((word & 0xff) << 24) | ((word & 0xff00) << 8) |
		((word >> 8) & 0xff00) | ((word >> 24) & 0xff);
#endif
}

static inline uint32_t iio_htobe32(uint32_t word)
{
	return iio_be32toh(word);
}

/*
 * If these structures are updated, the qsort functions defined in sort.c
 * may need to be updated.
 */

struct iio_context_pdata;
struct iio_device_pdata;
struct iio_channel_pdata;
struct iio_directory;
struct iio_module;

struct iio_channel_attr {
	char *name;
	char *filename;
};

struct iio_context {
	struct iio_context_pdata *pdata;
	const struct iio_backend_ops *ops;
	const char *name;
	char *description;

	unsigned int major;
	unsigned int minor;
	char *git_tag;

	struct iio_device **devices;
	unsigned int nb_devices;

	char *xml;

	char **attrs;
	char **values;
	unsigned int nb_attrs;

	struct iio_context_params params;

	struct iio_module *lib;
};

struct iio_channel {
	struct iio_device *dev;
	struct iio_channel_pdata *pdata;
	void *userdata;

	bool is_output;
	bool is_scan_element;
	struct iio_data_format format;
	char *name, *id;
	long index;
	enum iio_modifier modifier;
	enum iio_chan_type type;

	struct iio_channel_attr *attrs;
	unsigned int nb_attrs;

	unsigned int number;
};

struct iio_dev_attrs {
	char **names;
	unsigned int num;
};

struct iio_device {
	const struct iio_context *ctx;
	struct iio_device_pdata *pdata;
	void *userdata;

	char *name, *id, *label;

	struct iio_dev_attrs attrs;
	struct iio_dev_attrs buffer_attrs;
	struct iio_dev_attrs debug_attrs;

	struct iio_channel **channels;
	unsigned int nb_channels;

	struct iio_channels_mask *mask;
};

struct iio_buffer {
	const struct iio_device *dev;
	void *buffer, *userdata;
	size_t length, data_length;

	struct iio_channels_mask *mask;
	unsigned int dev_sample_size;
	unsigned int sample_size;
	bool dev_is_high_speed;
};

struct iio_context_info {
	char *description;
	char *uri;
};

struct iio_channels_mask {
	size_t words;
	uint32_t mask[];
};

struct iio_channels_mask *iio_create_channels_mask(unsigned int nb_channels);

int iio_channels_mask_copy(struct iio_channels_mask *dst,
			   const struct iio_channels_mask *src);

static inline bool
iio_channels_mask_test_bit(const struct iio_channels_mask *mask,
			   unsigned int bit)
{
	return mask->mask[BIT_WORD(bit)] & BIT_MASK(bit);
}

static inline void
iio_channels_mask_set_bit(struct iio_channels_mask *mask, unsigned int bit)
{
	mask->mask[BIT_WORD(bit)] |= BIT_MASK(bit);
}

static inline void
iio_channels_mask_clear_bit(struct iio_channels_mask *mask, unsigned int bit)
{
	mask->mask[BIT_WORD(bit)] &= ~BIT_MASK(bit);
}

struct iio_module * iio_open_module(const struct iio_context_params *params,
				    const char *name);
void iio_release_module(struct iio_module *module);

const struct iio_backend * iio_module_get_backend(struct iio_module *module);

void free_channel(struct iio_channel *chn);
void free_device(struct iio_device *dev);

ssize_t iio_snprintf_channel_xml(char *str, ssize_t slen,
				 const struct iio_channel *chn);
ssize_t iio_snprintf_device_xml(char *str, ssize_t slen,
				const struct iio_device *dev);

int iio_context_init(struct iio_context *ctx);

bool iio_device_is_tx(const struct iio_device *dev);
int iio_device_open(const struct iio_device *dev,
		size_t samples_count, bool cyclic);
int iio_device_close(const struct iio_device *dev);
int iio_device_set_blocking_mode(const struct iio_device *dev, bool blocking);
ssize_t iio_device_read_raw(const struct iio_device *dev,
		void *dst, size_t len, struct iio_channels_mask *mask);
ssize_t iio_device_write_raw(const struct iio_device *dev,
		const void *src, size_t len);
int iio_device_get_poll_fd(const struct iio_device *dev);

int read_double(const char *str, double *val);
int write_double(char *buf, size_t len, double val);

bool iio_list_has_elem(const char *list, const char *elem);

struct iio_context * xml_create_context_mem(const struct iio_context_params *params,
					    const char *xml, size_t len);
struct iio_context *
iio_create_dynamic_context(const struct iio_context_params *params,
			   const char *uri);
int iio_dynamic_scan(const struct iio_context_params *params,
		     struct iio_scan *ctx, const char *backends);

void iio_channel_init_finalize(struct iio_channel *chn);
unsigned int find_channel_modifier(const char *s, size_t *len_p);

char *iio_strndup(const char *str, size_t n);
char *iio_strtok_r(char *str, const char *delim, char **saveptr);
char * iio_getenv (char * envvar);

int iio_context_add_device(struct iio_context *ctx, struct iio_device *dev);

int iio_context_add_attr(struct iio_context *ctx,
		const char *key, const char *value);

int add_iio_dev_attr(struct iio_device *dev, struct iio_dev_attrs *attrs,
		     const char *attr, const char *type);

__cnst const struct iio_context_params *get_default_params(void);

extern const struct iio_backend iio_ip_backend;
extern const struct iio_backend iio_local_backend;
extern const struct iio_backend iio_serial_backend;
extern const struct iio_backend iio_usb_backend;
extern const struct iio_backend iio_xml_backend;

extern const struct iio_backend *iio_backends[];
extern const unsigned int iio_backends_size;

ssize_t iio_xml_print_and_sanitized_param(char *ptr, ssize_t len,
					  const char *before, char *param,
					  const char *after);

static inline void iio_update_xml_indexes(ssize_t ret, char **ptr, ssize_t *len,
					  ssize_t *alen)
{
	if (*ptr) {
		*ptr += ret;
		*len -= ret;
	}
	*alen += ret;
}

bool iio_channel_is_hwmon(const char *id);

#endif /* __IIO_PRIVATE_H__ */
