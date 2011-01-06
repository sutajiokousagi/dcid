/*
 * dcid_return_codes.c
 *
 * Aaron "Caustik" Robinson
 * (c) Copyright Chumby Industries, 2007
 * All rights reserved
 *
 * This module implements the DCID return code lookup table, which is provided
 * for convienence during debugging.
 */

#include "dcid_interface.h"

char *DCID_RETURN_CODE_LOOKUP[0x07] =
{
    "DCID_OK",
    "DCID_FAIL",
    "DCID_NOTIMPL",
    "DCID_INVALID_PARAM",
    "DCID_OUT_OF_MEMORY",
    "DCID_ACCESS_DENIED",
    "DCID_INVALID_CALL"
};
