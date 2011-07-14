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


#if defined(CNPLATFORM_netv) || defined(CNPLATFORM_wintergrasp)
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
/*************************************************************************/
#define ESD_CONFIG_AREA_PART1_OFFSET    0xc000
// pragma pack() not supported on all platforms, so we make everything dword-aligned using arrays
// WARNING: we're being lazy here and assuming that the platform any utility using this
// is running on little-endian! Otherwise you'll need to convert u32 values
typedef union {
        char name[4];
        unsigned int uname;
} block_def_name;

typedef struct _block_def {
        unsigned int offset;            // Offset from start of partition 1; if 0xffffffff, end of block table
        unsigned int length;            // Length of block in bytes
        unsigned char block_ver[4];     // Version of this block data, e.g. 1,0,0,0
        block_def_name n;               // Name of block, e.g. "krnA" (not NULL-terminated, a-z, A-Z, 0-9 and non-escape symbols allowed)
} block_def;

typedef struct _config_area {
        char sig[4];    // 'C','f','g','*'
        unsigned char area_version[4];  // 1,0,0,0
        unsigned char active_index[4];  // element 0 is 0 if krnA active, 1 if krnB; elements 1-3 are padding
        unsigned char updating[4];      // element 0 is 1 if update in progress; elements 1-3 are padding
        char last_update[16];           // NULL-terminated version of last successful update, e.g. "1.7.1892"
        unsigned int p1_offset;         // Offset in bytes from start of device to start of partition 1
        char factory_data[220];         // Data recorded in manufacturing in format KEY=VALUE<newline>...
        char configname[128];           // NULL-terminated CONFIGNAME of current build, e.g. "silvermoon_sd"
        unsigned char unused2[128];
        unsigned char mbr_backup[512];  // Backup copy of MBR
        block_def block_table[64];      // Block table entries ending with offset==0xffffffff
        unsigned char unused3[0];
} config_area;

static int seek_config_block(dcid_t *p_dcid, char *name) {
    int block;
    config_area cfg;

    if (p_dcid->device_file == -1) {
        perror("Unable to open config block device");
        return 0;
    }

    /* Seek to config table */
    if (-1 == lseek(p_dcid->device_file, ESD_CONFIG_AREA_PART1_OFFSET, SEEK_SET)) {
        perror("Unable to seek to config area");
        goto out;
    }

    /* Read config table */
    if (-1 == read(p_dcid->device_file, &cfg, sizeof(cfg))) {
        perror("Unable to read config area");
        goto out;
    }

    /* Locate cpid block */
    for (block=0; block < sizeof(cfg.block_table) / sizeof(cfg.block_table[0]); block++) {
        if (!memcmp(cfg.block_table[block].n.name, name, 4)) {

            /* Seek to specified block */
            if (-1 == lseek(p_dcid->device_file, cfg.block_table[block].offset, SEEK_SET)) {
                perror("Unable to seek to cpid block");
                goto out;
            }

            return 1;
        }
    }

out:
    return 0;
}
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

#if defined(CNPLATFORM_netv) || defined(CNPLATFORM_wintergrasp)
    if (!seek_config_block(p_dcid, "dcid"))
        return DCID_FAIL;
#endif


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


#if defined(CNPLATFORM_netv) || defined(CNPLATFORM_wintergrasp)
        int ret = 0;
        if(-1 == write(p_dcid->device_file, (uint8_t *)&cur, sizeof(uint8_t))) {
            perror("Unable to write");
            return DCID_FAIL;
        }
#endif

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

#if defined(CNPLATFORM_netv) || defined(CNPLATFORM_wintergrasp)
    int ret = 0;
    if (!seek_config_block(p_dcid, "dcid"))
        return DCID_FAIL;
    if (-1 == lseek(p_dcid->device_file, ed.address, SEEK_CUR)) {
        perror("Unable to seek");
        return DCID_FAIL;
    }
    if (-1 == read(p_dcid->device_file, &ed.data, sizeof(uint8_t))) {
        perror("Unable to read");
        return DCID_FAIL;
    }
#endif

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

