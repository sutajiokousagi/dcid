/*
 * dcid_utility.c
 *
 * Aaron "Caustik" Robinson
 * (c) Copyright Chumby Industries, 2007
 * All rights reserved
 */

#include "dcid_utility.h"
#include "chumby_accel.h" // @note this should be imported at some point!

#include <sys/ioctl.h>
#include <string.h>

#ifdef CNPLATFORM_avlite
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#endif

#ifdef CNPLATFORM_falconwing
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <unistd.h>
#define DCID_EEPROM_ADDR (0xA8)
#define PAGE_MULTIPLIER 2
#endif

#ifdef CNPLATFORM_silvermoon
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <unistd.h>
#define DCID_EEPROM_ADDR (0x50)
#define PAGE_MULTIPLIER 1
#endif

int dcid_util_write_raw(dcid_t *p_dcid, unsigned int addr, uint8_t *raw_data, int *p_size)
{
    int ret = DCID_OK;

    /*! sanity check */
    if(p_size == 0) { return DCID_INVALID_PARAM; }

    unsigned int cur_addr = addr;

    for(cur_addr = addr; cur_addr < (addr + (*p_size)); cur_addr++)
    {
        ret = dcid_util_write_byte(p_dcid, cur_addr, raw_data[cur_addr - addr]);

        if(DCID_FAILED(ret)) 
        { 
            *p_size = (cur_addr - addr);
            break;
        }
    }

    return ret;
}

int dcid_util_read_raw(dcid_t *p_dcid, unsigned int addr, uint8_t *raw_data, int *p_size)
{
    int ret = DCID_OK;

    /*! sanity check */
    if(p_size == 0) { return DCID_INVALID_PARAM; }

    unsigned int cur_addr = addr;

    for(cur_addr = addr; cur_addr < (addr + (*p_size)); cur_addr++)
    {
        ret = dcid_util_read_byte(p_dcid, cur_addr, &raw_data[cur_addr - addr]);

        if(DCID_FAILED(ret))
        {
            *p_size = (cur_addr - addr);
            break;
        }
    }

    return ret;
}

int dcid_util_write_flush(dcid_t *p_dcid)
{
    int v;

    for(v=0;v<=DCID_MAX_ADDRESS;v++)
    {
        uint16_t cur = p_dcid->write_cache[v];

        /*! only write if cache is dirty */
        if(cur > 255) { continue; }

#if defined(CNPLATFORM_avlite)
        int ret = 0;
        if(-1 == lseek(p_dcid->device_file, v, SEEK_SET)) {
            perror("Unable to seek");
            return DCID_FAIL;
        }
        if(-1 == write(p_dcid->device_file, (uint8_t *)&cur, sizeof(uint8_t))) {
            perror("Unable to write");
            return DCID_FAIL;
        }
#endif // defined(CNPLATFORM_avlite)

#if defined(CNPLATFORM_falconwing) || defined(CNPLATFORM_silvermoon)
        unsigned char output[2];
        struct i2c_rdwr_ioctl_data packets;
        struct i2c_msg messages[1];
        int byte;
        int page;

        // On this chip, the upper two bits of the memory address are
        // represented in the i2c address, and the lower eight are clocked in
        // as the memory address.  This gives a crude mechanism for 4 pages of
        // 256 bytes each.
        byte = (v   ) & 0xff;
        page = (v>>8) & 0x03;

        output[0] = byte;
        output[1] = (uint8_t)cur;

        messages[0].addr    = DCID_EEPROM_ADDR + (page*PAGE_MULTIPLIER);
        messages[0].flags   = 0;
        messages[0].len     = sizeof(output);
        messages[0].buf     = output;

        packets.msgs    = messages;
        packets.nmsgs   = 1;
        if(ioctl(p_dcid->device_file, I2C_RDWR, &packets) < 0) {
			char error[128];
			snprintf(error, sizeof(error), "Unable to send v %d on page %d, byte %d\n", v, page, byte);
            perror(error);
            return DCID_FAIL;
        }
        usleep(3*1000);
        
        int ret = 0;
#endif


#if defined(CNPLATFORM_ironforge)
        struct eeprom_data ed = { .address = v, .data = (uint8_t)cur };
        int ret = ioctl(p_dcid->device_file, ACCEL_IOCTL_SETROM, &ed);
#endif

        if(ret != 0) { return DCID_FAIL; }

        /*! clear this cache position */
        p_dcid->write_cache[v] = -1;
    }

    return DCID_OK;
}

int dcid_util_write_byte(dcid_t *p_dcid, unsigned int addr, uint8_t byte_val)
{
    /*! fail if out of range */
    if(addr > DCID_MAX_ADDRESS) { return DCID_INVALID_PARAM; }

    /*! update write cache */
    p_dcid->write_cache[addr] = byte_val;

    return DCID_OK;
}

int dcid_util_read_byte(dcid_t *p_dcid, unsigned int addr, uint8_t *p_byte_ret)
{
    struct eeprom_data ed = { .address = addr, .data = 0 };

    /*! fail if out of range */
    if(ed.address > DCID_MAX_ADDRESS) { return DCID_FAIL; }
    
#if defined(CNPLATFORM_avlite)
    int ret = 0;
    if(-1 == lseek(p_dcid->device_file, addr, SEEK_SET)) {
        perror("Unable to seek");
        return DCID_FAIL;
    }
    if(-1 == read(p_dcid->device_file, &ed.data, sizeof(uint8_t))) {
        perror("Unable to read");
        return DCID_FAIL;
    }
#endif // defined(CNPLATFORM_avlite)

#if defined(CNPLATFORM_falconwing) || defined(CNPLATFORM_silvermoon)
    int byte = 0;
    int page = 0;
    unsigned char output, input;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    // On this chip, the upper two bits of the memory address are
    // represented in the i2c address, and the lower eight are clocked in
    // as the memory address.  This gives a crude mechanism for 4 pages of
    // 256 bytes each.
    byte = (addr   ) & 0xff;
    page = (addr>>8) & 0x03;

    output = byte;
    messages[0].addr    = DCID_EEPROM_ADDR + (page*PAGE_MULTIPLIER);
    messages[0].flags   = 0;
    messages[0].len     = sizeof(output);
    messages[0].buf     = &output;

    messages[1].addr    = DCID_EEPROM_ADDR + (page*PAGE_MULTIPLIER);
    messages[1].flags   = I2C_M_RD;
    messages[1].len     = sizeof(input);
    messages[1].buf     = &input;

    packets.msgs    = messages;
    packets.nmsgs   = 2;
    if(ioctl(p_dcid->device_file, I2C_RDWR, &packets) < 0) {
        perror("Failure");
        return DCID_FAIL;
    }
    
    ed.data = input;
    int ret = 0;
#endif

#if defined(CNPLATFORM_ironforge)
    int ret = ioctl(p_dcid->device_file, ACCEL_IOCTL_READROM, &ed);
#endif

    if(ret != 0) { return DCID_FAIL; }

    if(p_byte_ret != 0) { *p_byte_ret = ed.data; }

    return DCID_OK;
}

int dcid_util_write_uint16(dcid_t *p_dcid, unsigned int addr, uint16_t uint16_val)
{
    int ret = dcid_util_write_byte(p_dcid, addr, (uint16_val & 0xFF00) >> 8);

    if(DCID_FAILED(ret)) { return ret; }

    ret = dcid_util_write_byte(p_dcid, addr+1, uint16_val & 0x00FF);

    return ret;
}

int dcid_util_read_uint16(dcid_t *p_dcid, unsigned int addr, uint16_t *p_uint16_ret)
{
    uint8_t byte_l, byte_h;

    int ret = dcid_util_read_byte(p_dcid, addr, &byte_h);

    if(DCID_FAILED(ret)) { return ret; }

    ret = dcid_util_read_byte(p_dcid, addr+1, &byte_l);

    if(DCID_FAILED(ret)) { return ret; }
    
    *p_uint16_ret = (byte_h << 8) | byte_l;

    return DCID_OK;
}

