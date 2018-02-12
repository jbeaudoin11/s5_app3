#ifndef ACCEL_H
#define ACCEL_H

#include <cmath>
#include <algorithm>

#include "mbed.h"
#define PI 3.14159265
#define LPC_GPIO0_FIOPIN (int *) 0x2009C014
#define LPC_GPIO0_FIODIR (int *) 0x2009C000
#define LPC_PINCON_PINSEL1 (int *) 0x4002C004
#define LPC_PINCON_PINMODE1 (int *) 0x4002C044

enum AccRegister {
	STATUS_MMA8452Q = 0x00,
	OUT_X_MSB = 0x01,
	OUT_X_LSB = 0x02,
	OUT_Y_MSB = 0x03,
	OUT_Y_LSB = 0x04,
	OUT_Z_MSB = 0x05,
	OUT_Z_LSB = 0x06,
	SYSMOD = 0x0B,
	INT_SOURCE = 0x0C,
	WHO_AM_I = 0x0D,
	XYZ_DATA_CFG = 0x0E,
	HP_FILTER_CUTOFF = 0x0F,
	PL_STATUS = 0x10,
	PL_CFG = 0x11,
	PL_COUNT = 0x12,
	PL_BF_ZCOMP = 0x13,
	P_L_THS_REG = 0x14,
	FF_MT_CFG = 0x15,
	FF_MT_SRC = 0x16,
	FF_MT_THS = 0x17,
	FF_MT_COUNT = 0x18,
	TRANSIENT_CFG = 0x1D,
	TRANSIENT_SRC = 0x1E,
	TRANSIENT_THS = 0x1F,
	TRANSIENT_COUNT = 0x20,
	PULSE_CFG = 0x21,
	PULSE_SRC = 0x22,
	PULSE_THSX = 0x23,
	PULSE_THSY = 0x24,
	PULSE_THSZ = 0x25,
	PULSE_TMLT = 0x26,
	PULSE_LTCY = 0x27,
	PULSE_WIND = 0x28,
	ASLP_COUNT = 0x29,
	CTRL_REG1 = 0x2A,
	CTRL_REG2 = 0x2B,
	CTRL_REG3 = 0x2C,
	CTRL_REG4 = 0x2D,
	CTRL_REG5 = 0x2E,
	OFF_X = 0x2F,
	OFF_Y = 0x30,
	OFF_Z = 0x31
};

enum AccOdr {
	ODR_800 = 0,
	ODR_400 = 1 << 3,
	ODR_200 = 2 << 3,
	ODR_100 = 3 << 3,
	ODR_50 = 4 << 3,
	ODR_12 = 5 << 3,
	ODR_6 = 6 << 3,
	ODR_1 = 7 << 3
};
enum AccScale {
	SCALE_2G = 0,
	SCALE_4G = 1,
	SCALE_8G = 2
};

struct Accel {
		char _x;
		char _y;
		char _z;

		char _data_buffer[3];
	
		I2C *_accel_i2c;
		int _addr;
	
		void _initI2C();
		void _initGPIO();
	
		void _writeTo(AccRegister reg, char data);
		void _writeBufferTo(AccRegister reg, char *buffer, char len);
	
		char _read();
		char _readFrom(AccRegister reg);
		void _readBufferFrom(AccRegister reg, char *buffer, char len);

		void _standby();
		void _active();
	
		void _setScale(AccScale scale);
		void _setOdr(AccOdr odr);
		void _setMode();
		void _set8Bit();
		
		Accel(int addr);
	
		void update();
	
		signed char x();
		signed char y();
		signed char z();
};

#endif 
