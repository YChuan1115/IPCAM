#include<bcm2835.h>
#include<stdint.h>
#include<sys/time.h>
#include"libit_i2c.h"

#define PCA9685_REG_MODE1         0x00
#define PCA9685_REG_MODE2         0x01
#define PCA9685_REG_SUBADR1       0x02
#define PCA9685_REG_SUBADR2       0x03
#define PCA9685_REG_SUBADR3       0x04
#define PCA9685_REG_PRESCALE      0xFE
#define PCA9685_REG_LED0_ON_L     0x06
#define PCA9685_REG_LED0_ON_H     0x07
#define PCA9685_REG_LED0_OFF_L    0x08
#define PCA9685_REG_LED0_OFF_H    0x09
#define PCA9685_REG_ALL_LED_ON_L  0xFA
#define PCA9685_REG_ALL_LED_ON_H  0xFB
#define PCA9685_REG_ALL_LED_OFF_L 0xFC
#define PCA9685_REG_ALL_LED_OFF_H 0xFD

#define PCA9685_PWM_FREQUENCY     50.0

int  pca9685_init(uint8_t slave_addr, int freq);
void pca968_reset(uint8_t slave_addr);
void pca9685_setPWMChannel(uint8_t slave_addr,uint8_t channel, uint16_t on, uint16_t off);
void pca9685_setDutyCycle(uint8_t slave_addr,uint8_t channel, float duty_cycle);
void pca9685_setPrescale(uint8_t slave_addr, uint8_t pre_scale);
void pca9685_setPWMFreq(uint8_t slave_addr, int _freq);
void pca9685_delay_1ms(long int count);
void pca9685_wakeup(uint8_t slave_addr);
void pca9685_sleep(uint8_t slave_addr);

int pca9685_init(uint8_t slave_addr, int freq){

	int res = i2c_init();
	pca9685_setPWMFreq(slave_addr, freq);
	pca9685_wakeup(slave_addr);
	return res;
}

void pca9685_setPWMChannel(uint8_t slave_addr,uint8_t channel, uint16_t on, uint16_t off){
	uint8_t on_byte_low   = on & 0x00ff;
	uint8_t on_byte_high  = on >> 8;
	uint8_t off_byte_low  = off & 0x00ff;
	uint8_t off_byte_high = off >> 8;

	uint8_t on_reg_low  = channel * 4 + PCA9685_REG_LED0_ON_L;
	uint8_t on_reg_high = channel * 4 + PCA9685_REG_LED0_ON_H;

	uint8_t off_reg_low  = channel * 4 + PCA9685_REG_LED0_OFF_L;
	uint8_t off_reg_high = channel * 4 + PCA9685_REG_LED0_OFF_H;

	i2c_write_reg_byte(slave_addr, on_reg_low, on_byte_low);
	i2c_write_reg_byte(slave_addr, on_reg_high, on_byte_high);

	i2c_write_reg_byte(slave_addr, off_reg_low, off_byte_low);
	i2c_write_reg_byte(slave_addr, off_reg_high, off_byte_high);
}

void pca9685_setDutyCycle(uint8_t slave_addr,uint8_t channel, float duty_cycle){
	uint16_t count = 4096.0 / 100.0 * duty_cycle;
	//printf("count:%ld\n",count);
	pca9685_setPWMChannel(slave_addr, channel, 0, count );
}


void pca9685_setPrescale(uint8_t slave_addr, uint8_t pre_scale){
	uint8_t now_pre_scale = i2c_read_reg_byte(slave_addr, PCA9685_REG_PRESCALE);
	if(now_pre_scale != pre_scale){
		//printf("set pre %d\n" , pre_scale);
		uint8_t oldmode1 = i2c_read_reg_byte(slave_addr, PCA9685_REG_MODE1);
		uint8_t newmode1 = oldmode1 | (1<<4);
		//printf("oldmode1 : %d\n",oldmode1);
		//printf("newmode1 : %d\n",newmode1);
		i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE1, newmode1) ;
		i2c_write_reg_byte(slave_addr, PCA9685_REG_PRESCALE, pre_scale) ;
		i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE1, oldmode1) ;
		pca9685_delay_1ms(5);
		i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE1, oldmode1 | 0x80) ;
	}
}

// input 
void pca9685_setPWMFreq(uint8_t slave_addr, int _freq){
	//printf("set freq : %d\n", _freq);
	//printf("slave add: %d\n", slave_addr);
	uint8_t prescale = ( 25.e6/ 4096. / _freq - 1) ;
	pca9685_setPrescale(slave_addr, prescale);
}

void pca968_reset(uint8_t slave_addr){
	i2c_write_reg_byte(slave_addr, PCA9685_REG_ALL_LED_ON_L, 0 );
	i2c_write_reg_byte(slave_addr, PCA9685_REG_ALL_LED_ON_H, 0 );
	i2c_write_reg_byte(slave_addr, PCA9685_REG_ALL_LED_OFF_L, 0 );
	i2c_write_reg_byte(slave_addr, PCA9685_REG_ALL_LED_OFF_H, 0 );
	i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE2, 0x04 );
	i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE1, 0x01 );
	pca9685_delay_1ms(5);
	uint8_t mode1 = i2c_read_reg_byte(slave_addr, PCA9685_REG_MODE1);
	i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE1, mode1 &~ (0x08));
}


void pca9685_delay_1ms(long int count){
	struct timeval start;
	struct timeval now;
	long int diff_us = 0;
	count *= 1000;

	gettimeofday(&start,NULL);
	while(diff_us <= count){
		gettimeofday(&now,NULL);
		diff_us = 1000000 * (now.tv_sec - start.tv_sec) - (now.tv_usec - now.tv_usec);
	}
}

void pca9685_wakeup(uint8_t slave_addr)
{
	uint8_t oldmode1 = i2c_read_reg_byte(slave_addr, PCA9685_REG_MODE1);
	uint8_t newmode1 = oldmode1 &~ (1<<4) ;
	i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE1, newmode1);
}

void pca9685_sleep(uint8_t slave_addr){
	uint8_t oldmode1 = i2c_read_reg_byte(slave_addr, PCA9685_REG_MODE1);
	uint8_t newmode1 = oldmode1 | (1<<4) ;
	i2c_write_reg_byte(slave_addr, PCA9685_REG_MODE1, newmode1);
}
