#include<bcm2835.h>
#include<stdint.h>

#ifndef __LIBIT_I2C_H
#define __LIBIT_I2C_H

uint8_t i2c_init(){
	uint8_t res = bcm2835_init();
	if(!res){fprintf(stderr,"i2c init error\n"); return -1;}

	bcm2835_i2c_setClockDivider(BCM2835_I2C_CLOCK_DIVIDER_148); // generate the i2c clock (60ns==1.666MHz) from system clock normal base clock rate is 25MHz
	bcm2835_i2c_begin();
	return 0;
}

void i2c_end(){
	bcm2835_i2c_end();
}

uint8_t i2c_read_reg_byte(uint8_t slave_addr,uint8_t reg){
	uint8_t buf[10];
	bcm2835_i2c_setSlaveAddress(slave_addr);
	bcm2835_i2c_write(&reg,1);
	bcm2835_i2c_read(buf,1);
	return buf[0];
}

void i2c_write_reg_byte(uint8_t slave_addr, uint8_t reg, uint8_t data){
	uint8_t buf[2];
	//const char * bufPtr = buf;
	buf[0] = reg;
	buf[1] = data;

	bcm2835_i2c_setSlaveAddress(slave_addr);
	//bcm2835_i2c_write(&buf,2);
	bcm2835_i2c_write(buf,2);
}

#endif
