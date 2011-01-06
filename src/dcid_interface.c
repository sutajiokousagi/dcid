/*
 * dcid_interface.c
 *
 * Aaron "Caustik" Robinson
 * (c) Copyright Chumby Industries, 2007
 * All rights reserved
 */

#include "dcid_interface.h"
#include "dcid_utility.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ctype.h>

/*! utility function for recursively parsing XML tags */
static int recursive_tag_parse(dcid_t *p_dcid, char *xml_data, int *p_cur_pos, int stop_pos, int depth);
/*! utility function for recursively writing XML tags */
static int recursive_tag_write(dcid_t *p_dcid, char **p_xml_data, int *p_cur_pos, char *lastTagRec);

int dcid_create(struct _dcid_info_t *p_dcid_info, struct _dcid_t **pp_dcid)
{
    /*! sanity check - null ptr */
    if(pp_dcid == 0) { return DCID_INVALID_PARAM; }

    /*! allocate associated context */
    dcid_t *p_dcid = (dcid_t*)malloc(sizeof(dcid_t));

    /*! set context to default state */
    memset(p_dcid, 0, sizeof(dcid_t));
    /*! default state - invalid file */
    p_dcid->device_file = -1;
    /*! write cache - initially empty */
    p_dcid->write_cache = (uint16_t*)malloc((DCID_MAX_ADDRESS+1)*sizeof(uint16_t));
    memset(p_dcid->write_cache, 0, (DCID_MAX_ADDRESS+1)*sizeof(uint16_t));

    /*! return allocated context */
    *pp_dcid = p_dcid;

    return DCID_OK;
}

int dcid_close(struct _dcid_t *p_dcid)
{
    /*! sanity check - null ptr */
    if(p_dcid == 0) { return DCID_INVALID_PARAM; }

    /*! cleanup write cache */
    if(p_dcid->write_cache != 0)
    {
        /*! free associated memory */
        free(p_dcid->write_cache);
        p_dcid->write_cache = 0;
    }

    /*! cleanup dcid device file */
    if(p_dcid->device_file != -1)
    {
        /*! close device device file */
        close(p_dcid->device_file);
        p_dcid->device_file = 0;
    }

    /*! free associated context */
    free(p_dcid);

    return DCID_OK;
}

int dcid_init(struct _dcid_t *p_dcid, char *dcid_device_path)
{
    /*! sanity check - null ptr */
    if(p_dcid == 0) { return DCID_INVALID_PARAM; }

    /*! attempt to open dcid device */
    p_dcid->device_file = open(dcid_device_path, O_RDWR);
    
#if defined(CNPLATFORM_avlite)
    if(p_dcid->device_file == -1) {
        p_dcid->device_file = open(dcid_device_path, O_RDWR | O_CREAT, 0644);
        if(p_dcid->device_file != -1) {
            char data[2048];
            bzero(data, sizeof(data));
            write(p_dcid->device_file, data, sizeof(data));
        }
    }
#endif    

    /*! failed to open dcid device */
    if(p_dcid->device_file == -1) { return DCID_FAIL; }

    /*! we're all initialized now */
    p_dcid->is_initialized = 1;

    return DCID_OK;
}

int dcid_read_xml(struct _dcid_t *p_dcid, char *xml_data, int *p_size)
{
    /*! sanity check - null ptr */
    if(p_dcid == 0) { return DCID_INVALID_PARAM; }

    /*! sanity check - null ptr */
    if(p_size == 0) { return DCID_INVALID_PARAM; }

    int cur_pos = 0;

    /*! reset xml_data */
    xml_data[0] = '\0';

    /*! validate header */
    {
        int size = 4;

        uint8_t hdr[4] = { 's', 'e', 'x', 'i' };
        uint8_t chk[4] = { 0 };

        dcid_util_read_raw(p_dcid, cur_pos, chk, &size);

        /*! fail if header validation failed */
        if(memcmp(chk, hdr, 4) != 0)
        {
            return DCID_FAIL;
        }

        cur_pos += 4;
    }

    /*! obligatory XML version header */
    strcat(xml_data, "<?xml version='1.0'?>\n");

    /*! parse data blocks */
    recursive_tag_parse(p_dcid, xml_data, &cur_pos, 0, 0);

    /*! validate trailer */
    {
        int size = 4;

        uint8_t tlr[4] = { 'p', 'u', 's', '!' };
        uint8_t chk[4] = { 0 };

        dcid_util_read_raw(p_dcid, cur_pos, chk, &size);

        if(memcmp(chk, tlr, 4) != 0)
        {
            return DCID_FAIL;
        }

        cur_pos += 4;
    }

    *p_size = strlen(xml_data) + 1;

    return DCID_OK;
}

int dcid_write_xml(struct _dcid_t *p_dcid, char *xml_data, int *p_size)
{
    /*! sanity check - null ptr */
    if(p_dcid == 0) { return DCID_INVALID_PARAM; }

    /*! sanity check - null ptr */
    if(p_size == 0) { return DCID_INVALID_PARAM; }

    int cur_pos = 0;

    /*! write header */
    {
        int size = 4;

        uint8_t hdr[4] = { 's', 'e', 'x', 'i' };

        int ret = dcid_util_write_raw(p_dcid, cur_pos, hdr, &size);

        if(DCID_FAILED(ret)) { return ret; }

        cur_pos += 4;
    }

    /*! recursively write tags */
    {
        int ret = recursive_tag_write(p_dcid, &xml_data, &cur_pos, "");

        if(DCID_FAILED(ret)) { return ret; }
    }

    /*! write trailer */
    {
        int size = 4;

        uint8_t tlr[4] = { 'p', 'u', 's', '!' };

        int ret = dcid_util_write_raw(p_dcid, cur_pos, tlr, &size);

        if(DCID_FAILED(ret)) { return ret; }

        cur_pos += 4;
    }

    /*! attempt to flush write cache */
    {
        int ret = dcid_util_write_flush(p_dcid);

        if(DCID_FAILED(ret)) { return ret; }
    }

    return DCID_OK;
}

static int recursive_tag_parse(dcid_t *p_dcid, char *xml_data, int *p_cur_pos, int stop_pos, int depth)
{
    do
    {
        uint16_t tag_size = 0;
        char tag_name[5] = { 0 };
        int rec = 0, v = 0;

        /*! read size */
        {
            int ret = dcid_util_read_uint16(p_dcid, *p_cur_pos, &tag_size);

            if(DCID_FAILED(ret)) { return ret; }

            rec = tag_size & 0x80;
            tag_size = tag_size & 0x7F;

            *p_cur_pos += 2;
        }

        /*! read tag */
        {
            int size = 4;

            int ret = dcid_util_read_raw(p_dcid, *p_cur_pos, (uint8_t*)&tag_name, &size);

            if(DCID_FAILED(ret)) { return ret; }

            *p_cur_pos += 4;
        }

        /*! write out opening tag */
        for(v=0;v<depth;v++) { strcat(xml_data, "  "); }
        strcat(xml_data, "<");
        strcat(xml_data, tag_name);
        strcat(xml_data, ">");

        if(rec) 
        { 
            strcat(xml_data, "\n"); 
            recursive_tag_parse(p_dcid, xml_data, p_cur_pos, *p_cur_pos + tag_size - 6, depth+1);
        }
        else
        {
            /*! read data */
            for(v=0;v<tag_size-6;v++)
            {
                uint8_t cur_byte;

                int ret = dcid_util_read_byte(p_dcid, *p_cur_pos, &cur_byte);

                if(DCID_FAILED(ret)) { return ret; }

                *p_cur_pos += 1;

                char buff[3] = { 0 };
                sprintf(buff, "%.02X", cur_byte);
                strcat(xml_data, buff);
            }
        }

        /*! write out closing tag */
        if(rec) { for(v=0;v<depth;v++) { strcat(xml_data, "  "); } }
        strcat(xml_data, "</");
        strcat(xml_data, tag_name);
        strcat(xml_data, ">\n");
    }
    while(*p_cur_pos < stop_pos);

    return DCID_OK;
}

static int recursive_tag_write(dcid_t *p_dcid, char **p_xml_data, int *p_cur_pos, char *lastTagRec)
{
    /*! we need to remember the last tag in order to validate start and close tag syntax */
    char lastTag[4] = { 0 };

    int v, skip_end_tag = 0;

    /*! remember last tag */
    strncpy(lastTag, lastTagRec, 4);

    while(1)
    {
        char *raw_tag_beg, *raw_tag_end;

        int size_addr = *p_cur_pos;

        /*! end of XML data */
        if((*p_xml_data)[0] == '\0') { break; }

        /*! locate next tag */
        raw_tag_beg = strchr(*p_xml_data, '<');

        /*! if no begin tag was found, give up */
        if(raw_tag_beg == 0) { break; }

        raw_tag_end = strchr(raw_tag_beg, '>');

        /*! if no end tag was found, give up */
        if(raw_tag_end == 0) { break; }

        /*! skip over <? tags */
        if(raw_tag_beg[1] == '?') 
        {
            *p_xml_data = raw_tag_end + 1;
            continue;
        }

        /*! finished with this level of recursion on close tag */
        if(raw_tag_beg[1] == '/')
        {
            *p_xml_data = raw_tag_end + 1;
            /*! detect mismatch between beginning/ending tags */
            if(strncmp(&raw_tag_beg[2], lastTag, 4) != 0) { return DCID_FAIL; }
            /*! detect if this should end this level of recursion, or just one tag */
            if(skip_end_tag) { strncpy(lastTag, lastTagRec, 4); skip_end_tag = 0; continue; }
            return DCID_OK; 
        }

        /*! ensure that this node name has exactly 4 characters */
        if( (raw_tag_end - raw_tag_beg) != 5) { return DCID_FAIL; }

        /*! skip over size field */
        *p_cur_pos += 2;

        /*! parse tag name */
        for(v=1;&raw_tag_beg[v] != raw_tag_end;v++)
        {
            dcid_util_write_byte(p_dcid, (*p_cur_pos)++, raw_tag_beg[v]);
        }

        /*! remember last tag */
        strncpy(lastTag, &raw_tag_beg[1], 4);

        /*! skip past tag end */
        *p_xml_data = raw_tag_end + 1;

        /*! throw away all whitespace */
        while(isspace(**p_xml_data)) { (*p_xml_data)++; }

        uint16_t chunk_size = 0;

        /*! if we have hex data, this is not a container */
        if(isxdigit(**p_xml_data))
        {
            unsigned int cur_byte = 0;

            while(sscanf(*p_xml_data, "%02X", &cur_byte) == 1)
            {
                dcid_util_write_byte(p_dcid, (*p_cur_pos)++, cur_byte);
                *p_xml_data += 2;
            }

            skip_end_tag = 1;

            chunk_size = (uint16_t)(*p_cur_pos - size_addr);
        }
        else
        {
            /*! remember last position, because if we have not found any more tags, we
             *  do not want to set the recursive flag for this node */
            int last_pos = *p_cur_pos;

            int ret = recursive_tag_write(p_dcid, p_xml_data, p_cur_pos, lastTag);

            if(DCID_FAILED(ret)) { return ret; }

            /*! restore correct lastTag */
            strncpy(lastTag, lastTagRec, 4);

            chunk_size = (uint16_t)(*p_cur_pos - size_addr);

            /*! tag this chunk as a container, if appropriate */
            if(*p_cur_pos > last_pos) { chunk_size |= 0x80; }
        }

        /*! write size field */
        dcid_util_write_uint16(p_dcid, size_addr, chunk_size);
    }

    return DCID_OK;
}

