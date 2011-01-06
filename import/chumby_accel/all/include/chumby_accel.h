/*
       chumby_accel.h
       bunnie -- March 2007 -- 2.0 -- port to Ironforge

       This file is part of the chumby accelerometer driver in the linux kernel.
       Copyright (c) Chumby Industries, 2007

       The accelerometer driver is free software; you can redistribute it and/or modify
       it under the terms of the GNU General Public License as published by
       the Free Software Foundation; either version 2 of the License, or
       (at your option) any later version.

       The accelerometer driver is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
       GNU General Public License for more details.

       You should have received a copy of the GNU General Public License
       along with the Chumby; if not, write to the Free Software
       Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <linux/ioctl.h>

#define ADC_CHSEL(x) (x)
#define ADC_XCH  0
#define ADC_YCH  2
#define ADC_ZCH  1

// always transition through CS3 because we can update both bits at once
// and spurious transitions on CS3 are "harmless" by design
#define CSPI_SS0_PIN  28
#define CSPI_SS1_PIN  27

#define ACCEL_SEL_CS3   imx_gpio_write( GPIO_PORTD | CSPI_SS1_PIN, 1 ); imx_gpio_write( GPIO_PORTD | CSPI_SS0_PIN, 1 );
#define ACCEL_SEL_CS2   imx_gpio_write( GPIO_PORTD | CSPI_SS1_PIN, 1 ); imx_gpio_write( GPIO_PORTD | CSPI_SS0_PIN, 0 );
#define ACCEL_SEL_CS1   imx_gpio_write( GPIO_PORTD | CSPI_SS1_PIN, 0 ); imx_gpio_write( GPIO_PORTD | CSPI_SS0_PIN, 1 );
#define ACCEL_SEL_CS0   imx_gpio_write( GPIO_PORTD | CSPI_SS1_PIN, 0 ); imx_gpio_write( GPIO_PORTD | CSPI_SS0_PIN, 0 );

#define ACCEL_IS_OPEN  0x1
#define DCID_IS_OPEN  0x1

#define ACCEL_IOCTL_MAGIC    'C'

#define SPIROM_WREN_CMD  0x06
#define SPIROM_WRDI_CMD  0x04
#define SPIROM_RDSR_CMD  0x05
#define SPIROM_WRSR_CMD  0x01
#define SPIROM_READ_CMD  0x03
#define SPIROM_WRITE_CMD 0x02

#define DCID_REV_LOC 0x300
#define DCID_REV_LEN 4

#define DCID_SERIAL_LOC  0x304
#define DCID_SERIAL_LEN  12     // serial number length in bytes

#define VERS_LEN (DCID_SERIAL_LEN + 1) // space for the null character


/**
 *  Defines for each of the commands. Note that since we want to reduce
 *  the possibility that a user mode program gets out of sync with a given 
 *  driver, we explicitly assign a value to each enumeration. This makes
 *  it more difficult to stick new ioctl's in the middle of the list.
 */

typedef enum
  {
    ACCEL_CMD_FIRST           = 0x80, // just a placeholder, not a command

    ACCEL_CMD_SETROM          = 0x80,
    ACCEL_CMD_READROM         = 0x81, 
    ACCEL_CMD_LOCKROM         = 0x82, 
    ACCEL_CMD_UNLOCKROM       = 0x83, 
    ACCEL_CMD_READSTAT        = 0x84, 

    /* Insert new ioctls here                                               */

    ACCEL_CMD_LAST,                  // another placeholder
} ACCEL_CMD;

/*
 * Our ioctl commands
 */

#define ACCEL_IOCTL_SETROM       _IOWR(  ACCEL_IOCTL_MAGIC, ACCEL_CMD_SETROM, int )      // arg is int
#define ACCEL_IOCTL_READROM      _IOWR(  ACCEL_IOCTL_MAGIC, ACCEL_CMD_READROM, int )     // arg is int
#define ACCEL_IOCTL_READSTAT     _IOWR(  ACCEL_IOCTL_MAGIC, ACCEL_CMD_READSTAT, int )     // arg is int
#define ACCEL_IOCTL_LOCKROM      _IOWR(  ACCEL_IOCTL_MAGIC, ACCEL_CMD_LOCKROM, int ) 
#define ACCEL_IOCTL_UNLOCKROM    _IOWR(  ACCEL_IOCTL_MAGIC, ACCEL_CMD_UNLOCKROM, int )

/*
 * The following are for entries in /proc/sys/chumbend
 */
#define CTL_CHUMACCEL    0x4348554F  /* 'CHUP' in hex form */

enum
{
    CTL_CHUMACCEL_DEBUG_TRACE    = 101,
    CTL_CHUMACCEL_DEBUG_IOCTL    = 102,
    CTL_CHUMACCEL_DEBUG_ERROR    = 103,
    CTL_CHUMACCEL_PWMVAL         = 104,
    CTL_CHUMACCEL_OUCHVAL        = 105,
    CTL_CHUMACCEL_BREAKVAL       = 106,
    CTL_CHUMACCEL_CLEAR          = 107,
    CTL_CHUMACCEL_ROMACT         = 108,
    CTL_CHUMDCID_VERS            = 109,
    CTL_CHUMDCID_SERIAL          = 110,
    CTL_CHUMDCID_CORE_VERS       = 111,
    CTL_CHUMDCID_CORE_SERIAL     = 112,
    CTL_CHUMDCID_EMPTY           = 113,
};

// data structures
//#define ACCEL_VERSION_NUM 1     // experimental katamari version
#define ACCEL_VERSION_NUM 3   // ironforge release with Kionix accelerometer

#define GRANGE 2500       // kionix is +/- 2g with reporting range up to 2.5g
struct accelReadData {
  unsigned int version;
  unsigned int timestamp;
  unsigned int inst[3];  // x, y, z
  unsigned int avg[3];
  unsigned int impact[3]; // values for the last impact peak acceleration
  unsigned int impactTime;
  unsigned int impactHint;
  unsigned int gRange;    // g range in milli-g's
};

struct eeprom_data {
  unsigned int address;  // address of read or write
  unsigned char data;    // data to be written (or read data upon return)
};

