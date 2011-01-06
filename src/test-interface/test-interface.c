/*
 * test-interface.c
 *
 * Aaron "Caustik" Robinson
 * (c) Copyright Chumby Industries, 2007
 * All rights reserved
 *
 * This module defines the entry point for the dcid test application.
 */

#include "dcid_interface.h"

#include <stdio.h>
#include <malloc.h>
#include <memory.h>

/*! serial port device path */
#if defined(CNPLATFORM_falconwing) || defined(CNPLATFORM_silvermoon)
#define DCID_DEVICE_PATH "/dev/i2c-0"
#else
#define DCID_DEVICE_PATH "/dev/dcid"
#endif

int main(int argc, char **argv)
{
    /*! default at failure */
    int main_ret = 1;

    /*! temporary buffer */
    char *tmp_buffer = (char*)malloc(DCID_MAX_XML_SIZE);

    /*! DCID instance */
    dcid_t *p_dcid = 0;

    /*! create DCID instance */
    {
        dcid_info_t dcid_info = { 0 };

        int ret = dcid_create(&dcid_info, &p_dcid);

        if(DCID_FAILED(ret)) 
        { 
            fprintf(stderr, "Error: dcid_create failed (%s)\n", DCID_RETURN_CODE_LOOKUP[ret]);
            goto cleanup;
        }
    }

    /*! initialize DCID instance */
    {
        int ret = dcid_init(p_dcid, DCID_DEVICE_PATH);

        if(DCID_FAILED(ret))
        {
            fprintf(stderr, "Error: dcid_init failed (%s)\n", DCID_RETURN_CODE_LOOKUP[ret]);
            goto cleanup;
        }
    }

    printf("Testing good XML...\n");

    /*! test good XML, with as many annoyances as possible */
    {
        static const char *test_xml = "<nod1> \r\n <nod2> \n\r\t00112233445566778899AABBCCDDEEFF??$*</nod2>\r<nod3>\n<nod4>FFAA</nod4> <nod5>EEBB</nod5></nod3></nod1>";
        static const char *test_xml_out = "<?xml version='1.0'?>\n<nod1>\n  <nod2>00112233445566778899AABBCCDDEEFF</nod2>\n  <nod3>\n    <nod4>FFAA</nod4>\n    <nod5>EEBB</nod5>\n  </nod3>\n</nod1>\n";

        strcpy(tmp_buffer, test_xml);

        int size = strlen(tmp_buffer)+1;

        int ret = dcid_write_xml(p_dcid, tmp_buffer, &size);

        if(DCID_FAILED(ret))
        {
            fprintf(stderr, "Error: dcid_write_xml failed with good XML:\n %s\n", tmp_buffer);
            goto cleanup;
        }

        size = strlen(tmp_buffer)+1;

        /*! clear out old XML, to get fresh results */
        memset(tmp_buffer, 0, size);

        ret = dcid_read_xml(p_dcid, tmp_buffer, &size);

        if(DCID_FAILED(ret))
        {
            fprintf(stderr, "Error: dcid_read_xml failed!\n");
            goto cleanup;
        }

        if(strcmp(tmp_buffer, test_xml_out) != 0)
        {
            fprintf(stderr, "Error: dcid_read_xml gave the wrong XML, expected:\n%s\ngot:\n%s\n", test_xml_out, tmp_buffer);
            goto cleanup;
        }
    }

    printf("Testing Malformed XML...\n");

    /*! test malformed XML, with more than 4 characters per node */
    {
        strcpy(tmp_buffer, "<nod1><nod2bad></nod2bad></nod1>");

        int size = strlen(tmp_buffer)+1;

        int ret = dcid_write_xml(p_dcid, tmp_buffer, &size);

        if(DCID_SUCCESS(ret))
        {
            fprintf(stderr, "Error: dcid_write_xml succeeded with bad XML:\n %s\n", tmp_buffer);
            goto cleanup;
        }
    }

    printf("All Tests Passed!\n");

    main_ret = 0;

cleanup:

    /*! cleanup temp buffer */
    if(tmp_buffer != 0)
    {
        free(tmp_buffer);
        tmp_buffer = 0;
    }

    /*! cleanup DCID instance */
    if(p_dcid != 0)
    {
        int ret = dcid_close(p_dcid);

        if(DCID_FAILED(ret))
        {
            fprintf(stderr, "Warning: dcid_close failed (0x%.08X)\n", ret);
            return 1;
        }
    }

    return main_ret;
}

