/*
 * dcid_utility.h
 *
 * Aaron "Caustik" Robinson
 * (c) Copyright Chumby Industries, 2007
 * All rights reserved
 *
 * This API defines utility functions for dcid_interface.
 */

#ifndef DCID_UTILITY_H
#define DCID_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dcid_interface.h"

/*! write raw bytes to dcid device */
int dcid_util_write_raw(dcid_t *p_dcid, unsigned int addr, uint8_t *raw_data, int *p_size);

/*! read raw bytes from dcid device */
int dcid_util_read_raw(dcid_t *p_dcid, unsigned int addr, uint8_t *raw_data, int *p_size);

/*! write a single raw byte to dcid device */
int dcid_util_write_byte(dcid_t *p_dcid, unsigned int addr, uint8_t byte_val);

/*! read a single raw byte from dcid device */
int dcid_util_read_byte(dcid_t *p_dcid, unsigned int addr, uint8_t *p_byte_ret);

/*! write a single uint16 to dcid device */
int dcid_util_write_uint16(dcid_t *p_dcid, unsigned int addr, uint16_t uint16_val);

/*! read a single uint16 from dcid device */
int dcid_util_read_uint16(dcid_t *p_dcid, unsigned int addr, uint16_t *p_uint16_ret);

/*! flush write cache to device */
int dcid_util_write_flush(dcid_t *p_dcid);

#ifdef __cplusplus
}
#endif

#endif
