/*
 * 		Modifications to code done by: Lucas Burns
 * 		Complete credit for original code goes to Stefano Barbato, details below:
 */

/***************************************************************************
    copyright            : (C) by 2003-2004 Stefano Barbato
    email                : stefano@codesink.org
    $Id: 24cXX.c,v 1.5 2004/02/29 11:05:28 tat Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "24cXX.h"

C24CXX::C24CXX(const char *dev_fqn, int addr, int type)
{
	int n = eeprom_open(dev_fqn, addr, type);
	if (n >= 0) {
		//::fprintf(stdout, "EEPROM successfully initialized.\n");
	} else {
		//::fprintf(stdout, "Error initializing EEPROM.\n");
	}

	// Debug code
//	::fprintf(stdout, "Dumping first 16 bytes of EEPROM...\n");
//	eeprom_read(0,  16, 1);
}

C24CXX::~C24CXX()
{
	//::fprintf(stdout, "Destroying C24CXX object...\n");
	::close(m_fd);
}

int C24CXX::i2c_write_1b(__u8 buf)
{
	int r;
	// we must simulate a plain I2C byte write with SMBus functions
	r = i2c_smbus_write_byte(m_fd, buf);
	if(r < 0)
		//::fprintf(stderr, "Error i2c_write_1b: %s\n", ::strerror(errno));
	::usleep(10);
	return r;
}

int C24CXX::i2c_write_2b(__u8 buf[2])
{
	int r;
	// we must simulate a plain I2C byte write with SMBus functions
	r = i2c_smbus_write_byte_data(m_fd, buf[0], buf[1]);
	if(r < 0)
		//::fprintf(stderr, "Error i2c_write_2b: %s\n", ::strerror(errno));
	::usleep(10);
	return r;
}

int C24CXX::i2c_write_3b(__u8 buf[3])
{
	int r;
	// we must simulate a plain I2C byte write with SMBus functions
	// the __u16 data field will be byte swapped by the SMBus protocol
	r = i2c_smbus_write_word_data(m_fd, buf[0], buf[2] << 8 | buf[1]);
	if(r < 0)
		//::fprintf(stderr, "Error i2c_write_3b: %s\n", ::strerror(errno));
	::usleep(10);
	return r;
}


#define CHECK_I2C_FUNC( var, label ) \
	do { 	if(0 == (var & label)) { \
		::fprintf(stderr, "\nError: " \
			#label " function is required. Program halted.\n\n"); \
		::exit(1); } \
	} while(0);

int C24CXX::eeprom_open(const char *dev_fqn, int addr, int type)
{
	int funcs, fd, r;
	fd = 0;
	m_dev = 0;

	fd = ::open(dev_fqn, O_RDWR);
	if(fd <= 0)
	{
		//::fprintf(stderr, "Error eeprom_open: %s\n", ::strerror(errno));
		return -1;
	}

	// get funcs list
	if((r = ::ioctl(fd, I2C_FUNCS, &funcs) < 0))
	{
		//::fprintf(stderr, "Error eeprom_open: %s\n", ::strerror(errno));
		return -1;
	}

	// check for req funcs
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_QUICK );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_READ_BYTE );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_WRITE_BYTE );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_READ_BYTE_DATA );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_WRITE_BYTE_DATA );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_READ_WORD_DATA );
	CHECK_I2C_FUNC( funcs, I2C_FUNC_SMBUS_WRITE_WORD_DATA );

	// set working device
	if( ( r = ::ioctl(fd, I2C_SLAVE, addr)) < 0)
	{
		//::fprintf(stderr, "Error eeprom_open: %s\n", strerror(errno));
		return -1;
	}



	m_fd = fd;
	m_addr = addr;
	m_dev = dev_fqn;
	m_type = type;

	//::printf("Device %s at address 0x%02X initialized under file descriptor %d.\n", dev_fqn, addr, fd);

	return 0;
}

int C24CXX::eeprom_close()
{
	::close(m_fd);
	m_fd = -1;
	m_dev = 0;
	m_type = EEPROM_TYPE_UNKNOWN;
	return 0;
}

#if 0
int eeprom_24c32_write_byte(__u16 mem_addr, __u8 data)
{
	__u8 buf[3] = { (mem_addr >> 8) & 0x00ff, mem_addr & 0x00ff, data };
	return i2c_write_3b(buf);
}


int eeprom_24c32_read_current_byte()
{
	ioctl(m_fd, BLKFLSBUF); // clear kernel read buffer
	return i2c_smbus_read_byte(m_fd);
}

int eeprom_24c32_read_byte(__u16 mem_addr)
{
	int r;
	ioctl(m_fd, BLKFLSBUF); // clear kernel read buffer
	__u8 buf[2] = { (mem_addr >> 8) & 0x0ff, mem_addr & 0x0ff };
	r = i2c_write_2b(buf);
	if (r < 0)
		return r;
	r = i2c_smbus_read_byte(m_fd);
	return r;
}
#endif


int C24CXX::eeprom_read_current_byte()
{
	::ioctl(m_fd, BLKFLSBUF); // clear kernel read buffer
	return i2c_smbus_read_byte(m_fd);
}

int C24CXX::eeprom_read_byte(__u16 mem_addr)
{
	int r;
	::ioctl(m_fd, BLKFLSBUF); // clear kernel read buffer
	if(m_type == EEPROM_TYPE_8BIT_ADDR)
	{
		__u8 buf =  static_cast<__u8>(mem_addr & 0x00FF);
		r = i2c_write_1b(buf);
	} else if(m_type == EEPROM_TYPE_16BIT_ADDR) {
		__u8 buf[2] = { static_cast<__u8>((mem_addr >> 8) & 0xFFU), static_cast<__u8>(mem_addr & 0x00FFU) };
		r = i2c_write_2b(buf);
	} else {
		//::fprintf(stderr, "ERR: unknown eeprom type\n");
		return -1;
	}
	if (r < 0)
		return r;
	r = i2c_smbus_read_byte(m_fd);
	return r;
}

int C24CXX::eeprom_write_byte(__u16 mem_addr, __u8 data)
{
	if(m_type == EEPROM_TYPE_8BIT_ADDR) {
		__u8 buf[2] = { static_cast<__u8>(mem_addr & 0x00ff), data };
		return i2c_write_2b(buf);
	} else if(m_type == EEPROM_TYPE_16BIT_ADDR) {
		__u8 buf[3] =
			{ static_cast<__u8>((mem_addr >> 8) & 0x00ff), static_cast<__u8>(mem_addr & 0x00ff), data };
		return i2c_write_3b(buf);
	}
	//::fprintf(stderr, "ERR: unknown eeprom type\n");
	return -1;
}

int C24CXX::eeprom_wait_ready(int max_ms_to_wait)
{
	while (max_ms_to_wait > 0) {
		//int r = i2c_smbus_write_quick(eeprom->fd, 0);
		int r = i2c_smbus_write_quick(m_fd, 0);
		if (r == 0) {
			return r;
		}
		::usleep(1000);
		--max_ms_to_wait;
	}
	return -1;
}

int C24CXX::eeprom_write(int addr, unsigned char *data, int size, int timeout)
{
	//::printf("Writing %d bytes to EEPROM starting at address 0x%02X...\n", size, addr);
	for (int i = 0; i < size; i++) {
		if (&data[i] != NULL) {
			eeprom_write_byte(addr + i, data[i]);
			if (eeprom_wait_ready(timeout) < 0) {
				//::printf("In C24CXX::eeprom_write: Writing timed out. Aborting write...\n");
				return -1;
			}
		}
	}

	return 0;
}

int C24CXX::getFileDescriptor()
{
	return m_fd;
}

bool C24CXX::eeprom_check()
{
	int n = eeprom_read_byte(0x00);
	if (n < 0) return false;
	return true;
}
