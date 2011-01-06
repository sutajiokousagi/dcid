/*
 * main.c
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
#endif

#if defined(CNPLATFORM_ironforge)
#define DCID_DEVICE_PATH "/dev/dcid"
#endif

#if defined(CNPLATFORM_avlite)
#define DCID_DEVICE_PATH "/psp/dcid.bin"
#endif

/*! print program usage screen */
static void show_usage();

int main(int argc, char **argv)
{
    /*! default at failure */
    int main_ret = 1;

    /*! input file, if specified */
    FILE *inp_file = 0;

    /*! output file, if specified */
    FILE *out_file = 0;

    /*! temporary buffer */
    char *tmp_buffer = (char*)malloc(DCID_MAX_XML_SIZE);

    /*! DCID instance */
    dcid_t *p_dcid = 0;

    /*! options for stdout */
    int print_usage = 0;

    /*! print usage if there are no arguments */
    if(argc <= 1) { print_usage = 1; }

    /*! parse command line */
    {
        int cur_arg = 0;

        for(cur_arg = 1; cur_arg < argc; cur_arg++)
        {
            /*! expect all options to begin with '-' character */
            if(argv[cur_arg][0] != '-') 
            { 
                fprintf(stderr, "Warning: Unrecognized option \"%s\"\n", argv[cur_arg]);
                continue; 
            }

            /*! process command options */
            switch(argv[cur_arg][1])
            {
#ifdef DCID_ALLOW_WRITE
                case 'w':
                {
                    /*! skip over to filename */
                    if(++cur_arg >= argc) { break; }

                    /*! attempt to open inpt file */
                    inp_file = fopen(argv[cur_arg], "rt");

                    /*! report file open error to user */
                    if(inp_file == 0) 
                    { 
                        fprintf(stderr, "Error: Could not open \"%s\" for reading.\n", argv[cur_arg]); 
                    }
                }
                break;
#endif
                case 'r':
                {
                    /*! skip over to filename */
                    if(++cur_arg >= argc) { break; }

                    /*! attempt to open output file */
                    out_file = fopen(argv[cur_arg], "wt");

                    /*! report file open error to user */
                    if(out_file == 0) 
                    { 
                        fprintf(stderr, "Error: Could not open \"%s\" for writing\n", argv[cur_arg]); 
                    }
                }
                break;

#ifdef DCID_ALLOW_WRITE
                case 'i':
                {
                    if(inp_file == 0) { inp_file = stdin; }
                }
                break;
#endif
                case 'o':
                {
                    if(out_file == 0) { out_file = stdout; }
                }
                break;

                case '-':
                    print_usage = 1;
                    break;

                default:
                {
                    fprintf(stderr, "Warning: Unrecognized option \"%s\"\n", argv[cur_arg]);
                }
                break;
            }
        }
    }

    /*! optionally, show usage */
    if(print_usage)
    {
        show_usage();
        goto cleanup;
    }

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

    /*! optionally write dcid device data */
    if(inp_file != 0)
    {
        int size = 0;

        /*! read up to max buffer size minus null terminator */
        {
            size_t ret = fread(tmp_buffer, 1, DCID_MAX_XML_SIZE-1, inp_file);

            /*! update size */
            if(ret > 0) { size = ret; }

            /*! append null terminator */
            tmp_buffer[size] = '\0';
        }

        /*! write input data to device */
        {
            int ret = dcid_write_xml(p_dcid, tmp_buffer, &size);

            if(DCID_FAILED(ret))
            {
                fprintf(stderr, "Error: dcid_write_xml failed (%s)\n", DCID_RETURN_CODE_LOOKUP[ret]);
                goto cleanup;
            }
        }
    }

    /*! optionally read dcid device data */
    if(out_file != 0)
    {
        int size = DCID_MAX_XML_SIZE;

        /*! read up to max buffer size */
        {
            int ret = dcid_read_xml(p_dcid, tmp_buffer, &size);

            if(DCID_FAILED(ret))
            {
                fprintf(stderr, "Error: dcid_read_xml failed (%s)\n", DCID_RETURN_CODE_LOOKUP[ret]);
                goto cleanup;
            }
        }

        /*! write output data to out_file */
        if(size > 0)
        {
            size_t ret = fwrite(tmp_buffer, 1, size-1, out_file);

            if(ret != size-1) 
            { 
                fprintf(stderr, "Error: fwrite returned %d, expected %d\n", ret, size-1); 
                goto cleanup;
            }
        }
    }

    main_ret = 0;

cleanup:

    /*! cleanup input file handle(s) */
    if( (inp_file != 0) && (inp_file != stdin) )
    {
        fclose(inp_file);
        inp_file = 0;
    }

    /*! cleanup file handle(s) */
    if( (out_file != 0) && (out_file != stdout) )
    {
        fclose(out_file);
        out_file = 0;
    }

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

static void show_usage()
{
    printf("DCID 1.0 [caustik@chumby.com]\n");
    printf("\n");
#ifdef DCID_ALLOW_WRITE
    printf("Usage : dcid [--help] | [-r <FILE>] [-w <FILE>] [-i] [-o]\n");
    printf("\n");
    printf("Read/Write from DCID device\n");
    printf("\n");
    printf("Options:\n");
    printf("\n");
    printf("    --help      Display this help screen\n");    
    printf("\n");
    printf("    -w <FILE>   Write contents of FILE to \"%s\"\n", DCID_DEVICE_PATH);
    printf("    -r <FILE>   Write contents of \"%s\" to FILE\n", DCID_DEVICE_PATH);
    printf("    -i          Write contents of stdin to \"%s\" (ignored if valid -w specified)\n", DCID_DEVICE_PATH);
    printf("    -o          Write contents of \"%s\" to stdout (ignored if valid -r specified)\n", DCID_DEVICE_PATH);
#else
    printf("Usage : dcid [-r FILE] [-o]\n");
    printf("\n");
    printf("Read from DCID device\n");
    printf("\n");
    printf("    --help      Display this help screen");
    printf("\n");
    printf("Options:\n");
    printf("    -r <FILE>   Write contents of \"%s\" to FILE\n", DCID_DEVICE_PATH);
    printf("    -o          Write contents of \"%s\" to stdout\n", DCID_DEVICE_PATH);
#endif
    printf("\n");
    return;
}

