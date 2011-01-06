/*
 * dcid_interface.h
 *
 * Aaron "Caustik" Robinson
 * (c) Copyright Chumby Industries, 2007
 * All rights reserved
 *
 * This API defines an interface for accessing the daughter card ID information.
 *
 * DCID := Daughter Card ID (this module)
 */

#ifndef DCID_INTERFACE_H
#define DCID_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*! \name forward declarations */
/*! \{ */
struct _dcid_info_t;
struct _dcid_t;
/*! \} */

/*!

 Create an instance of the Daughter Card ID Interface.

  @param p_dcid_info (INP) - DCID Info used to initialize DCID instance
  @param pp_dcid (OUT) - Pointer to DCID instance
  @return DCID_OK for success, otherwise DCID_ error code

*/

int dcid_create(struct _dcid_info_t *p_dcid_info, struct _dcid_t **pp_dcid);

/*!

 Closes an instance of the Daughter Card ID Interface.

  @param p_dcid (INP) - DCID instance
  @return DCID_OK for success, otherwise DCID_ error code

 */

int dcid_close(struct _dcid_t *p_dcid);

/*!

 Initialize an instance of the Daughter Card ID Interface. Must be called before
 any other API calls, aside from dcid_create and dcid_close.

  @param p_dcid (INP) - DCID instance
  @param dcid_device_path (INP) - Path to proper device (e.g. "/dev/dcid")
  @return DCID_OK for success, otherwise DCID_ error code

 */

int dcid_init(struct _dcid_t *p_dcid, char *dcid_device_path);

/*!

 Read XML data from the Daughter Card ID Interface.

  @param p_dcid (INP) - DCID instance
  @param xml_data (OUT) - XML data in ASCII char encoding
  @param p_size (INP/OUT) - INP: Max size, in bytes, to write to xml_data buffer.
                            OUT: Returns number of bytes written (incl null terminator).
  @return DCID_OK for success, otherwise DCID_ error code

 */

int dcid_read_xml(struct _dcid_t *p_dcid, char *xml_data, int *p_size);

/*!

 Write XML data to the Daughter Card ID Interface.

  @param p_dcid (INP) - DCID instance
  @param xml_data (OUT) - XML data in ASCII char encoding
  @param p_size (INP/OUT) - INP: Max size, in bytes, to write to DCID.
                            OUT: Returns number of bytes written (does not write null terminator).
  @return DCID_OK for success, otherwise DCID_ error code

 */

int dcid_write_xml(struct _dcid_t *p_dcid, char *xml_data, int *p_size);

/*! 

  @brief DCID instance

  This structure represents an instance of the DCID.

*/

typedef struct _dcid_t
{
    /*! dcid device file handle */
    int device_file;
    /*! initialization flag */
    int is_initialized;
    /*! write cache, to prevent partial writes. value above 255 implies no cached value */
    uint16_t *write_cache;
}
dcid_t;

/*! 

  @brief DCID information 

  This structure should be passed to dcid_create() in order to initialize
  a DCID instance. 

*/

typedef struct _dcid_info_t
{
    int dummy; /*!< temporary placeholder */
}
dcid_info_t;

/*! \name DCID sizes, in bytes */
/*! \{ */
#define DCID_MAX_XML_SIZE        0x1000  /*!< 4096 bytes, @todo finalize this max */
#define DCID_MAX_RAW_SIZE        0x0300  /*!< 768 bytes */
/*! \} */

/*! maximum address available for read/write from DCID */
#define DCID_MAX_ADDRESS (DCID_MAX_RAW_SIZE-1)

/*! \name DCID return codes */
/*! \{ */
#define DCID_OK                  0x0000  /*!< Success! */
#define DCID_FAIL                0x0001  /*!< Generic failure */
#define DCID_NOTIMPL             0x0002  /*!< Functionality not implemented */
#define DCID_INVALID_PARAM       0x0003  /*!< Invalid parameter */
#define DCID_OUT_OF_MEMORY       0x0004  /*!< Out of memory */
#define DCID_ACCESS_DENIED       0x0005  /*!< Access denied */
#define DCID_INVALID_CALL        0x0006  /*!< Invalid call */
/*! \} */

/*! \name DCID return code lookup table, for convienence */
/*! \{ */
char *DCID_RETURN_CODE_LOOKUP[0x07];
/*! \} */

/*! \name DCID return code helper functions */
/*! \{ */
/*! Detect success error code */
#define DCID_SUCCESS(x) (x == DCID_OK)
/*! Detect failure error code */
#define DCID_FAILED(x) (x != DCID_OK)
/*! \} */

#ifdef __cplusplus
}
#endif

#endif

