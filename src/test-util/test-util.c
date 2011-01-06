/*
 * test-util.c
 *
 * Aaron "Caustik" Robinson
 * (c) Copyright Chumby Industries, 2007
 * All rights reserved
 *
 * This module defines the entry point for the dcid test application.
 */

#include "dcid_interface.h"
#include "dcid_utility.h"

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
    void *tmp_buffer = malloc(DCID_MAX_XML_SIZE);

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
            perror("Failed");
            fprintf(stderr, "Error: dcid_init failed (%s)\n", DCID_RETURN_CODE_LOOKUP[ret]);
            goto cleanup;
        }
    }

    printf("Testing dcid_util_read_byte...\n");

    /*! test dcid_util_read_byte */
    {
        /*! test ranges - min, max, should_succeed */
        int test_range[3][3] =
        {
            { 0,                    DCID_MAX_ADDRESS+1, 1 },
            { DCID_MAX_ADDRESS+1,   DCID_MAX_ADDRESS*2, 0 },
            { -DCID_MAX_ADDRESS,    -1,                 0 }
        };

        int test_range_count = 3;

        int c, v;

        /*! iterate through each of the 3 sets of ranges */
        for(c=0;c<test_range_count;c++)
        {
            int min = test_range[c][0];
            int max = test_range[c][1];
            int should_succeed = test_range[c][2];

            printf("  Range %d -> %d should %s...", min, max, should_succeed ? "succeed" : "not succeed");
            fflush(stdout);

            for(v=test_range[c][0];v<test_range[c][1];v++)
            {
                uint8_t org_val = '\0';

                int state;

                for(state=0;state<2;state++)
                {
                    uint8_t val = '\0';

                    int ret = dcid_util_read_byte(p_dcid, v, &val);

                    if(should_succeed == 1)
                    {
                        if(DCID_FAILED(ret))
                        {
                            printf("Failed!\n");
                            fprintf(stderr, "Error: dcid_util_read_byte(0x%.08X, %d, 0x%.08X) := %d\n", (uint32_t)p_dcid, v, (uint32_t)&val, ret);
                            goto cleanup;
                        }
                    }
                    else
                    {
                        if(DCID_SUCCESS(ret))
                        {
                            printf("Failed!\n");
                            fprintf(stderr, "Error: dcid_util_read_byte(0x%.08X, %d, 0x%.08X) := %d\n", (uint32_t)p_dcid, v, (uint32_t)&val, ret);
                            goto cleanup;
                        }
                    }

                    /*! we have multiple states here. first time through, we want to store the original value,
                     *  and write a special value that is dependent on the address mod 255. next time, we want
                     *  to validate this value was read back correctly, and write the original value back. last,
                     *  we want to ensure the original value was restored. the total operation should leave the
                     *  memory bank as it was originally. */
                    switch(state)
                    {
                        case 0:
                        {
                            org_val = val;

                            ret = dcid_util_write_byte(p_dcid, v, v%255);
                        }
                        break;

                        case 1:
                        {
                            if(should_succeed && (val != v%255))
                            {
                                printf("Failed!\n");
                                fprintf(stderr, "Error: Address %d reported %d instead of %d (addr mod 255)\n", v, val, v%255);
                                goto cleanup;
                            }

                            ret = dcid_util_write_byte(p_dcid, v, org_val);
                        }
                        break;

                        case 2:
                        {
                            if(should_succeed && (val != org_val))
                            {
                                printf("Failed!\n");
                                fprintf(stderr, "Error: Address %d reported %d instead of %d (org_val)\n", v, val, org_val);
                                goto cleanup;
                            }
                        }
                        break;
                    }

                    if(should_succeed == 1)
                    {
                        if(DCID_FAILED(ret))
                        {
                            printf("Failed!\n");
                            fprintf(stderr, "Error: dcid_util_write_byte failed at address %d [ret := %d]\n", v, ret);
                            goto cleanup;
                        }
                    }
                    else
                    {
                        if(DCID_SUCCESS(ret))
                        {
                            printf("Failed!\n");
                            fprintf(stderr, "Error: dcid_util_write_byte incorrectly succeeded at address %d [ret := %d]\n", v, ret);
                            goto cleanup;
                        }
                    }

                    /*! don't forget to flush! */
                    dcid_util_write_flush(p_dcid);
                }
            }

            printf("Passed.\n");
        }
    }

    printf("Testing dcid_util_read_raw...\n");

    /*! test dcid_util_read_raw */
    {
        int size = DCID_MAX_ADDRESS+1;

        /*! test exact buffer size */
        int ret = dcid_util_read_raw(p_dcid, 0, tmp_buffer, &size);

        if(DCID_FAILED(ret))
        {
            fprintf(stderr, "Error: dcid_util_read_raw(0x%.08X, 0, 0x%.08X, %d) := %d\n", (uint32_t)p_dcid, (uint32_t)tmp_buffer, DCID_MAX_ADDRESS, ret);
            goto cleanup;
        }

        size = DCID_MAX_ADDRESS+2;

        /*! test single byte overflow */
        ret = dcid_util_read_raw(p_dcid, 0, tmp_buffer, &size);

        if(DCID_SUCCESS(ret))
        {
            fprintf(stderr, "Error: dcid_util_read_raw(0x%.08X, 0, 0x%.08X, %d) := %d\n", (uint32_t)p_dcid, (uint32_t)tmp_buffer, DCID_MAX_ADDRESS, ret);
            goto cleanup;
        }

        if(size != DCID_MAX_ADDRESS+1)
        {
            fprintf(stderr, "Error: size := %d\n", size);
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

