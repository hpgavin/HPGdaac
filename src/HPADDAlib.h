/* HPADDAlib.h
 * ADS1256 datasheet ... https://www.ti.com/lit/ds/symlink/ads1256.pdf 
 
MIT License

Copyright (c) 2018 shujima

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

https://github.com/shujima/HPADDAlibrary

 */


#ifndef _HPADDALIB_H_
#define _HPADDALIB_H_

#include <stdio.h>
#include <stdint.h>   // uint8_t, ...
#include <bcm2835.h>  // BroadCom SPI, I2C, and GPIO interface 

#define NUMCHNL 8         
#define ADMIN   0         /* min value returned by 24 bit A/D        */
#define ADMAX   0xFFFFFF  /* max value returned by 24 bit A/D        */
#define ADMID   0x800000  /* mid value returned by 24 bit A/D        */
#define DA_LO   0x0000    /* min input value accepted by 16 bit D/A  */
#define DA_HI   0xFFFF    /* max input value accepted by 16 bit D/A  */
#define DA_00   0x0000    /* D/A output corresponding to zero volts  */

#define SPI_DELAY 2       /* delay time for SPI transfers */


//  GPIO read and write functions 
#define GPIOwrite(_pin, _value) bcm2835_gpio_write(_pin, _value)
#define GPIOread(_pin) bcm2835_gpio_lev(_pin)

//  SPI  read and write functions
#define SPIwriteByte(__value) bcm2835_spi_transfer(__value)
#define SPIreadByte() bcm2835_spi_transfer(0xFF)

//  delay functions for milliseconds and microseconds
#define delay_ms(__xms) bcm2835_delay(__xms)
#define delay_us(__xms) bcm2835_delayMicroseconds(__xms)

//  GPIO pin configuration for HPADDAgc outputs
#define PIN_38 RPI_BPLUS_GPIO_J8_38            /* 20 */
#define PIN_40 RPI_BPLUS_GPIO_J8_40            /* 21 */


// Register definition  --- ADS1256 datasheet Page 30, Table 23. 
#define REG_STATUS   0x00
#define REG_MUX      0x01
#define REG_ADCON    0x02
#define REG_CSPEED   0x03
#define REG_IO       0x04
#define REG_OFC0     0x05 
#define REG_OFC1     0x06
#define REG_OFC2     0x07
#define REG_FSC0     0x08
#define REG_FSC1     0x09
#define REG_FSC2     0x0A

// ADS1256 Analog Input Channel Multiplexer (MUX) - datasheet page 31
#define ADS1256_AIN0_AINCOM 0B00001000
#define ADS1256_AIN1_AINCOM 0B00011000
#define ADS1256_AIN2_AINCOM 0B00101000
#define ADS1256_AIN3_AINCOM 0B00111000
#define ADS1256_AIN4_AINCOM 0B01001000
#define ADS1256_AIN5_AINCOM 0B01011000
#define ADS1256_AIN6_AINCOM 0B01101000
#define ADS1256_AIN7_AINCOM 0B01111000

// enums

// Command Definitions - datasheet page 34, Table 24
typedef enum {
	CMD_WAKEUP_00	= 0x00,		// Completes SYNC and exits standby mode
	CMD_RDATA	= 0x01,		// Read Data
	CMD_RDATAC	= 0x03,		// Read Data Continuously
	CMD_SDATAC	= 0x0F,		// Stop Read Data Continuously
	CMD_RREG	= 0x10,		// Read from REG rrrr
	CMD_WREG	= 0x50,		// Write to  REG rrrr
	CMD_SELFCAL	= 0xF0,		// Offset and Gain Self-Calibration
	CMD_SELFOCAL	= 0xF1,		// Offset Self-Calibration
	CMD_SELFGCAL	= 0xF2,		// Gain Self-Calibration
	CMD_SYSOCAL	= 0xF3,		// System Offset Calibration
	CMD_SYSGCAL	= 0xF4,		// System Gain Calibration
	CMD_SYNC	= 0xFC,		// Synchronize the A/D Conversion
	CMD_STANDBY	= 0xFD,		// Begin Standby Mode
	CMD_RESET	= 0xFE,		// Reset to Power-Up Values
	CMD_WAKEUP_FF	= 0xFF,		// Completes SYNC and exits standby mode
} ADS1256_COMMANDS;
 

// ADS1256 Gain Codes - datsheet page 31
// Programmable gain amplifier levels with respective register values
#define UP_5_00V     (0x00)           // UniPolar 0 - 5.00 V
#define UP_2_50V     (0x01)           // UniPolar 0 - 2.50 V
#define UP_1_25V     (0x02)           // UniPolar 0 - 1.25 V
#define UP_0_625V    (0x03)           // UniPolar 0 - 0.625 V
#define UP_0_3125V   (0x04)           // UniPolar 0 - 0.3125 V
#define UP_0_15625V  (0x05)           // UniPolar 0 - 0.15625V
#define UP_0_078125V (0x06)           // UniPolar 0 - 0.078125V

/*
// ADS1256 Input Voltage Range
#define  ADS1256_VOLTRANGE_1        5.0
#define  ADS1256_VOLTRANGE_2        2.5
#define  ADS1256_VOLTRANGE_4        1.25
#define  ADS1256_VOLTRANGE_8        0.625
#define  ADS1256_VOLTRANGE_16       0.3125
#define  ADS1256_VOLTRANGE_32       0.15625
#define  ADS1256_VOLTRANGE_64       0.078125
*/

/* ADS1256 Input Gain
typedef enum
{
  ADS1256_GAIN_1           = 0,           // GAIN  1 
  ADS1256_GAIN_2           = 1,           // GAIN  2 
  ADS1256_GAIN_4           = 2,           // GAIN  4 
  ADS1256_GAIN_8           = 3,           // GAIN  8 
  ADS1256_GAIN_16          = 4,           // GAIN 16 
  ADS1256_GAIN_32          = 5,           // GAIN 32 
  ADS1256_GAIN_64          = 6,           // GAIN 64 
} ADS1256_GAIN_E;
*/



// Available conversion speeds with their respective register values
// datasheet page 18
typedef enum {
	ADS1256_30000_SPS	= 0xF0,	 //     1 Average (Bypassed) Default
	ADS1256_15000_SPS	= 0xE0,	 //     2 Averages
	ADS1256_7500_SPS	= 0xD0,	 //     4 Averages
	ADS1256_3750_SPS	= 0xC0,	 //     8 Averages
	ADS1256_2000_SPS	= 0xB0,	 //    15 Averages
	ADS1256_1000_SPS	= 0xA1,	 //    30 Averages
	ADS1256_500_SPS		= 0x92,	 //    60 Averages
	ADS1256_100_SPS		= 0x82,	 //   300 Averages
	ADS1256_60_SPS		= 0x72,	 //   500 Averages
	ADS1256_50_SPS		= 0x63,	 //   600 Averages
	ADS1256_30_SPS		= 0x53,	 //  1000 Averages
	ADS1256_25_SPS		= 0x43,	 //  1200 Averages
	ADS1256_15_SPS		= 0x33,	 //  2000 Averages
	ADS1256_10_SPS		= 0x23,	 //  3000 Averages  ?? 0x20 ??
	ADS1256_5_SPS		= 0x13,	 //  6000 Averages
	ADS1256_2d5_SPS		= 0x03,	 // 12000 Averages
} ADS1256_CSPEED;


// ADS1256 AD conversion frequencies
#define ADS1256_30000_FRQ	 30000.0
#define ADS1256_15000_FRQ	 15000.0
#define ADS1256_7500_FRQ	  7500.0
#define ADS1256_3750_FRQ	  3750.0
#define ADS1256_2000_FRQ	  2000.0
#define ADS1256_1000_FRQ	  1000.0
#define ADS1256_500_FRQ		   500.0	
#define ADS1256_100_FRQ		   100.0
#define ADS1256_60_FRQ		    60.0
#define ADS1256_50_FRQ		    50.0
#define ADS1256_30_FRQ		    30.0
#define ADS1256_25_FRQ		    25.0
#define ADS1256_15_FRQ		    15.0
#define ADS1256_10_FRQ		    10.0
#define ADS1256_5_FRQ		     5.0	
#define ADS1256_2d5_FRQ		     2.5	

/* ADS1256 conversion speeds
typedef enum
{
  ADS1256_30000SPS = 0, ADS1256_15000SPS, ADS1256_7500SPS, ADS1256_3750SPS, ADS1256_2000SPS, ADS1256_1000SPS, ADS1256_500SPS, ADS1256_100SPS, ADS1256_60SPS, ADS1256_50SPS, ADS1256_30SPS, ADS1256_25SPS, ADS1256_15SPS, ADS1256_10SPS, ADS1256_5SPS, ADS1256_2d5SPS, ADS1256_CSPEED_MAX
}ADS1256_CSPEED_E;

static const uint8_t s_tabDataRate[ADS1256_CSPEED_MAX] =
{
    0xF0,0xE0,0xD0,0xC0, 0xB0,0xA1,0x92,0x82,0x72, 0x63,0x53,0x43,0x33,0x20,0x13, 0x03
}; 
*/

//#####################################################################
//
//  Prototype Declaration of Functions 
//
//#####################################################################


// Common

//void  delay_us(uint64_t micros);

void  resetADS1256(void);
int   initHPADDAboard(void);
void  closeHPADDAboard(void);


// Get AD
int32_t ADS1256_GetADC(int8_t positive_no , int8_t negative_no );
double ADS1256_IntToVolt(int32_t value , double vref);
void ADS1256_ChannelScan(unsigned nChnl, uint8_t muxCode[], uint8_t rangeCode[], int32_t adScan[]);


// DA
void DAC8532_Write( int dac_channel , unsigned int val);
unsigned int DAC8532_VoltToValue( double volt , double volt_ref);

// Print AD
void ADS1256_PrintAllValue();
void ADS1256_PrintAllValueDiff();
void ADS1256_PrintAllReg();

// AD settings
float ADS1256_SetDigitizationRate( float drate );

uint8_t ADS1256_range_code ( float rangeValue );
float ADS1256_range_value ( uint8_t rangeCode );
void ADS1256_set_gain ( uint8_t rangeCode );

uint8_t ADS1256_ReadChipID(void);
void ADS1256_WriteReg(uint8_t _RegID, uint8_t _RegValue);
uint8_t ADS1256_ReadReg(uint8_t _RegID);

// Privates
void ADS1256_WaitDRDY(void);

#endif
