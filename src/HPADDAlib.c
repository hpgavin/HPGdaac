/*
 * HPADDAlib.c
 * This program is based on an official "ads1256_test.c".
 * This program is released under the MIT license.
 
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

#include "HPADDAlib.h"
#include "../../HPGnumlib/HPGutil.h"

// Globals from the bcm2835 library
const uint8_t DA_SPI_CS = RPI_GPIO_P1_16;
const uint8_t AD_SPI_CS = RPI_GPIO_P1_15;
const uint8_t AD_RESET  = RPI_GPIO_P1_12;
const uint8_t AD_DRDY   = RPI_GPIO_P1_11;


// to store the data read from the AD data register 
// int32_t    registerData = 0;

/* name: resetADS1256
 * function: reset the ADS1256 chip
 * parameter:
 * Info: t16 delay time > 4 x tau_clkin = 0.5 use
 */
void resetADS1256(void)
{
    fprintf(stderr," ADS1256 reset "); fflush(stderr);
    bcm2835_gpio_write( AD_RESET , HIGH );
    delay_ms(50);
    bcm2835_gpio_write( AD_RESET , LOW ); 
    delay_ms(50); 
    bcm2835_gpio_write( AD_RESET , HIGH );
    delay_ms(50); 
    fprintf(stderr," . . . . . . . . . . . . . . . . . . . . . . . . .  success \n");
}


/*   name: initHPADDAboard
 *   function:  init ADS1256 & DAC8532 
 *   parameter: NULL
 *   The return value:  0:successful 1:Abnormal
 */
int initHPADDAboard()
{
    //start bcm2835 library
    fprintf(stderr," bcm2835 init  "); fflush(stderr);
    int initstate = bcm2835_init();
    if (initstate < 0) { 
      color(1); color(31); 
      fprintf(stderr," . . . . . . . . . . . . . . . . . (initstate = %d)  failed \n",initstate);
      color(1); color(37);
      return 1;
    } else {
      fprintf(stderr," . . . . . . . . . . . . . . . . . (initstate = %d)  success \n",initstate);
    }

    // SPI settings
    fprintf(stderr," bcm2835 SPI init  "); fflush(stderr);
    bcm2835_spi_begin();    // start SPI interface, set SPI pin for reuse 
    // ---   SPI.beginTransaction(speed,bitorder,mode)
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST );   
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1); 
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);  //6.250Mhz on Raspberry Pi 3 
    fprintf(stderr," . . . . . . . . . . . . . . . . . . . . . . .  success \n");


    fprintf(stderr," bcm2835 GPIO pin configuration  "); fflush(stderr);
    // GPIO output pin selection 
    // ... AD pin settings
    bcm2835_gpio_fsel( AD_RESET   , BCM2835_GPIO_FSEL_OUTP );
    bcm2835_gpio_fsel( AD_SPI_CS  , BCM2835_GPIO_FSEL_OUTP );
    bcm2835_gpio_write( AD_SPI_CS , HIGH);
    bcm2835_gpio_fsel( AD_DRDY    , BCM2835_GPIO_FSEL_INPT );
    bcm2835_gpio_set_pud( AD_DRDY , BCM2835_GPIO_PUD_UP );
    delay_us(10);               // wait t6 ??
    // ... DA pin settings
    bcm2835_gpio_fsel( DA_SPI_CS  , BCM2835_GPIO_FSEL_OUTP );
    bcm2835_gpio_write( DA_SPI_CS , HIGH);
    // set PIN_38 and PIN_40 to outputs
    bcm2835_gpio_fsel( PIN_38 , BCM2835_GPIO_FSEL_OUTP );
    bcm2835_gpio_fsel( PIN_40 , BCM2835_GPIO_FSEL_OUTP );
    fprintf(stderr," . . . . . . . . . . . . . . . .  success \n");

    resetADS1256();

    fprintf(stderr," ADS1256 chip ID read  "); fflush(stderr);
    if( ADS1256_ReadChipID() == 3){
        fprintf(stderr," . . . . . . . . . . . . . . . . . . . . .  success \n");
    } else {
        color(1); color(31); 
        printf(" . . . . . . . . . . . . . . . . . . . . .  failed \n");
        printf(" . . . try rebooting the Raspberry PI . . . \n");
        color(1); color(37); 
        return 1;
    }

    fprintf(stderr," ADS1256 set registers "); fflush(stderr);
    // set ADS1256 registers ( just in case ...)
    bcm2835_gpio_write( DA_SPI_CS , HIGH);
    bcm2835_gpio_write( AD_SPI_CS , LOW);  // AD1256 SPI Start
    delay_us(10);               // wait t6  ??

    bcm2835_spi_transfer(0xFE); // reset
    delay_ms(2);                // minimum 0.6 ms required for Reset to finish
    bcm2835_spi_transfer(0x0F); // issues SDATAC to do the reset
    delay_us(10);               // wait t6
    bcm2835_spi_transfer(CMD_WREG | 0);  // Command : Write from 0 register
    delay_ms(2);               
    bcm2835_spi_transfer(4);  // Number of Registers to write - 1 = 5 - 1 = 4
    delay_ms(500); 
    bcm2835_spi_transfer(0x01);    // register 0x00 : STATUS
    delay_ms(200);                 // ??
    bcm2835_spi_transfer(0x01);    // register 0x01 : MUX 
    delay_ms(200);                 // ??
    bcm2835_spi_transfer(0x20);    // register 0x02 : ADCON (reset value = 0x20)
    delay_ms(200);                 // ??
    bcm2835_spi_transfer(0xF0);    // register 0x03 : CSPEED
    delay_ms(500);                 // ??
    bcm2835_spi_transfer(0xE0);    // register 0x04 : IO
    delay_ms(200);                 // ??

    fprintf(stderr," . . . . . . . . . . . . . . . . . . . . .  success \n");

    bcm2835_gpio_write( AD_SPI_CS, HIGH );  // ADS1256 SPI end

    ADS1256_PrintAllReg();


/*
    bcm2835_spi_end();         // end SPI transaction ??
*/


    return 0;
}


/*   name: closeHPADDAboard
 *   function:  close SPI, ADS1256 & DAC8532 
 *   parameter: NULL
 *   The return value:  NULL
 */
void closeHPADDAboard()
{
    bcm2835_spi_end();
    bcm2835_close();
}



//#####################################################################
//  AD Functions
//

/*   name: ADS1256_ReadReg
 *   function: Read  the corresponding register
 *   parameter: RegID: register  ID
 *   The return value: read register value
 */
uint8_t ADS1256_ReadReg(uint8_t RegID)
{
    uint8_t registerValueR;

    bcm2835_gpio_write(DA_SPI_CS,HIGH);

    while(bcm2835_gpio_lev(AD_DRDY)) { /* wait for DRDY to go low */ }

/*
    //SPI settings
    bcm2835_spi_begin();                     // begin SPI transaction
    // ---   SPI.beginTransaction(speed,bitorder,mode)
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST );   
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1); 
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);  //6.250Mhz on Raspberry Pi 3 
*/

    bcm2835_gpio_write(AD_SPI_CS,LOW);       // SPI  cs  = 0
    delay_us(10);                            // t6 delay
    bcm2835_spi_transfer(CMD_RREG | RegID);  // Read REGister command 
    delay_us(10);                            // t6 delay
    bcm2835_spi_transfer(0x00);              // number of bytes to read minus 1
    delay_us(10);                            // The minimum time delay 6.5us 
    registerValueR = bcm2835_spi_transfer(0xff);  // Read the register values 
    delay_us(10);                            // The minimum time delay 6.5us 
    bcm2835_gpio_write(AD_SPI_CS,HIGH);      // SPI   cs  = 1 
/*
    bcm2835_spi_end();                       // end SPI transaction
*/

    return registerValueR;
}


/*   name: ADS1256_WriteReg
 *   function: Write the corresponding register
 *   parameter: RegID: register  ID
 *              RegValue: register Value
 *   The return value: NULL
*/
void ADS1256_WriteReg(uint8_t RegID, uint8_t RegValue)
{
    bcm2835_gpio_write(DA_SPI_CS,HIGH);

    // relevant video: https://youtu.be/KQ0nWjM-MtI
    while(bcm2835_gpio_lev(AD_DRDY)) { /* wait for DRDY to go low */ }

/*
    bcm2835_spi_begin();                     // begin SPI transaction
    // ---   SPI.beginTransaction(speed,bitorder,mode)
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST );   
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1); 
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);  //6.250Mhz on Raspberry Pi 3 
*/

    bcm2835_gpio_write(AD_SPI_CS,LOW);       // SPI  cs  = 0
    delay_us(10);                            // t6 delay
    bcm2835_spi_transfer(CMD_WREG | RegID);  // Write REGister command
    bcm2835_spi_transfer(0x00);              // number of bytes to write minus 1
    delay_us(10);                            // t6 delay
    bcm2835_spi_transfer(RegValue);          // send register value 
    bcm2835_gpio_write(AD_SPI_CS,HIGH);      // SPI   cs = 1
/*
    bcm2835_spi_end();                       // end SPI transaction
*/
}


/*   name: ADS1256_ReadChipID
 *   function: Read the chip ID
 *   parameter: _cmd : NULL
 *   The return value: four high status register
 */
uint8_t ADS1256_ReadChipID(void)
{
    uint8_t chipID;

    while(bcm2835_gpio_lev(AD_DRDY)) { /* wait for DRDY to go low */ }
    chipID = ADS1256_ReadReg(REG_STATUS);
    return (chipID >> 4);
}


/*
 *   name: ADS1256_PrintAllReg
 *   function:  print all regulator of ADS1256
 *   parameter: NULL
 *   The return value:  NULL
 * ------------------------------------------------------------------------- */
void ADS1256_PrintAllReg()
{
    const char regName[11][20] = {"STATUS", "MUX   ", "ADCON ", "CSPEED", "IO    ",
                            "OFC0  ", "OFC1  ", "OFC2  ", "FSC0  ", "FSC1  ", "FSC2  "};
    int reg;
    for (reg = 0 ; reg < 11 ; reg++)
        printf("%s : %4x\n", regName[reg] , ADS1256_ReadReg(reg));
}


/*   name: ADS1256_SetDigitizationRate
 *   function: set conversion speed of ADS1256
 *   parameter: speed : [conversions per second] (2.5 - 30000)
 *   This speed is for one input. It becomes later in the case of multi input.
 *   The return value: Actual set value
 */
float ADS1256_SetDigitizationRate( float drate )
{
    ADS1256_CSPEED set_drate_e = ADS1256_100_SPS;// default 1000 conversions/sec
    float set_drate_f = ADS1256_1000_FRQ;        // default 1000 conversions/sec

    if     (drate <=    2.5){
      set_drate_e = ADS1256_2d5_SPS;   
      set_drate_f = ADS1256_2d5_FRQ;   
    }
    else if(drate <=     5.0 ){
      set_drate_e = ADS1256_5_SPS;     
      set_drate_f = ADS1256_5_FRQ;     
    }
    else if(drate <=    10.0 ){
      set_drate_e = ADS1256_10_SPS;    
      set_drate_f = ADS1256_10_FRQ;    
    }
    else if(drate <=    15.0 ){
      set_drate_e = ADS1256_15_SPS;    
      set_drate_f = ADS1256_15_FRQ;    
    }
    else if(drate <=    25.0 ){
      set_drate_e = ADS1256_25_SPS;    
      set_drate_f = ADS1256_25_FRQ;    
    }
    else if(drate <=    30.0 ){
      set_drate_e = ADS1256_30_SPS;    
      set_drate_f = ADS1256_30_FRQ;    
    }
    else if(drate <=    50.0 ){
      set_drate_e = ADS1256_50_SPS;    
      set_drate_f = ADS1256_50_FRQ;    
    }
    else if(drate <=    60.0 ){
      set_drate_e = ADS1256_60_SPS;    
      set_drate_f = ADS1256_60_FRQ;    
    }
    else if(drate <=   100.0 ){
      set_drate_e = ADS1256_100_SPS;   
      set_drate_f = ADS1256_100_FRQ;   
    }
    else if(drate <=   500.0 ){
      set_drate_e = ADS1256_500_SPS;   
      set_drate_f = ADS1256_500_FRQ;   
    }
    else if(drate <=  1000.0 ){
      set_drate_e = ADS1256_1000_SPS;  
      set_drate_f = ADS1256_1000_FRQ;  
    }
    else if(drate <=  2000.0 ){
      set_drate_e = ADS1256_2000_SPS;  
      set_drate_f = ADS1256_2000_FRQ;  
    }
    else if(drate <=  3750.0 ){
      set_drate_e = ADS1256_3750_SPS;  
      set_drate_f = ADS1256_3750_FRQ;  
    }
    else if(drate <=  7500.0 ){
      set_drate_e = ADS1256_7500_SPS;  
      set_drate_f = ADS1256_7500_FRQ;  
    }
    else if(drate <= 15000.0 ){
      set_drate_e = ADS1256_15000_SPS; 
      set_drate_f = ADS1256_15000_FRQ; 
    }
    else if(drate >  15000.0 ){
      set_drate_e = ADS1256_30000_SPS; 
      set_drate_f = ADS1256_30000_FRQ; 
    }

    ADS1256_WriteReg( 3 , set_drate_e );

    return set_drate_f;
}


/*
ADS1256_RANGE_CODE  -  return the range code for the ADS1256 amplifier
-----------------------------------------------------------------------------*/
uint8_t ADS1256_range_code ( float rangeValue )
{
  uint8_t rangeCode = UP_5_00V;

  if (rangeValue <=  5.00)       rangeCode = UP_5_00V;
  if (rangeValue <=  2.50)       rangeCode = UP_2_50V;
  if (rangeValue <=  1.25)       rangeCode = UP_1_25V;
  if (rangeValue <=  0.625)      rangeCode = UP_0_625V;
  if (rangeValue <=  0.3125)     rangeCode = UP_0_3125V;
  if (rangeValue <=  0.15625)    rangeCode = UP_0_15625V;
  if (rangeValue <=  0.078125)   rangeCode = UP_0_078125V;

//printf("\n rangeValue = %f   rangeCode = %d \n", rangeValue, rangeCode );

  return(rangeCode);
}


/*
ADS1256_RANGE_VALUE -  return the floating point value of the bipolar range as a float
-----------------------------------------------------------------------------*/
float ADS1256_range_value ( uint8_t rangeCode )
{   
  switch(rangeCode) {
    case UP_5_00V:
      return(5.00);
      break;
    case UP_2_50V:
      return(2.50);
      break;
    case UP_1_25V:
      return(1.25);
      break;
    case UP_0_625V:
      return(0.625);
      break;
    case UP_0_3125V:
      return(0.3125);
      break;
    case UP_0_15625V:
      return(0.15625);
      break;
    case UP_0_078125V:
      return(0.078125);
      break;
    default:
      return(5.00);
      break;
  }
}


/*
ADS1256_SET_GAIN - set the gain for all channels on ADS1256
-----------------------------------------------------------------*/
void ADS1256_set_gain ( uint8_t rangeCode ) 
{
  ADS1256_WriteReg( 2 , (ADS1256_ReadReg(2) & 0xf8) | (rangeCode & 0x07) );
}


/*    name: ADS1256_GetADC
 *    function: read ADC value
 *    
 *   positive_no: input port no of positive side (0 - 7 or -1)
 *                For single-ended it should be set to 
 *                the channel number ... 0 - 7
 *                When set to -1 , AGND becomes positive input
 *   negative_no: input port no of negative side (-1 or 0 - 7)
 *                For single ended it should be set to -1
 *                For Differential Input, set to 0 - 7
 *    The return value:  ADC vaule (signed 32 bit integer)
 * 
 *    The return value:  ADC vaule 
 */
int32_t ADS1256_GetADC(int8_t positive_no , int8_t negative_no )
{
    int8_t positive_common =  0;    // Bit 7
    int8_t negative_common =  1;    // Bit 3
    int8_t mux_data = 0;
    int32_t registerData = 0x00000000; 

    /*
     set ADS1256 MUX for changing input of ADC 
    |---------------------------------------------------------------|
    | ADS1256 / Register / MUX (0x01)                               |
    |---------------------------------------------------------------|
    | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
    |---------------------------------------------------------------|
    | Bit 7-4 positive side input   | Bit 3-0 negative side input   |
    | 0000 to 0111 : AIN0 to AIN7   | 0000 to 0111 : AIN0 to AIN7   |
    | 1xxx : AINCOM (AGND)          | 1xxx : AINCOM (AGND)          |
    |---------------------------------------------------------------|
    */

   /*  check input port numbers  */
    if (positive_no < 0 || positive_no > 7)
    {
        positive_common = 1;
        if ( positive_no != -1 )perror("ADS1256_GetADC Illegal Input\n");
    }
    if (negative_no < 0 || negative_no > 7)
    {
        negative_common = 1;
        if ( negative_no != -1 )perror("ADS1256_GetADC Illegal Input\n");
    }
    mux_data = (positive_common << 7) | ((positive_no & 7) << 4) | (negative_common << 3) | ((negative_no & 7));

    while(bcm2835_gpio_lev(AD_DRDY)) { /* wait for DRDY to go low */ }

/*
    bcm2835_spi_begin();                     // begin SPI transaction
    // ---   SPI.beginTransaction(speed,bitorder,mode)
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST );   
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1); 
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);  //6.250Mhz on Raspberry Pi 3 
*/

    bcm2835_gpio_write(AD_SPI_CS,LOW);       // SPI  cs  = 0
    delay_us(10);                            // t6 delay

    bcm2835_spi_transfer(CMD_WREG | REG_MUX);// Write to MUX REGister
    bcm2835_spi_transfer(0x00);              // number of bytes to write minus 1
    delay_us(10);                            // t6 delay
    bcm2835_spi_transfer(mux_data);          // set the MUX
    delay_us(SPI_DELAY);
    bcm2835_spi_transfer(CMD_SYNC);          // Step 2.     
    delay_us(4);                 // t11 delay 24*tau = 3.125 us round up to 4 us
    bcm2835_spi_transfer(CMD_WAKEUP_FF);
    delay_us(SPI_DELAY);
    bcm2835_spi_transfer(CMD_RDATA);         // Step 3. 
    delay_us(5);                         // t6 delay (~6.51 us) p.34, fig.30

    // Read the sample result as 24 bits in three 8-bit bytes
    // step out the data: MSB | mid-byte | LSB,
    registerData |= bcm2835_spi_transfer(0x0F); // transfer MSB (high 8 bits) first 
    registerData <<= 8;                         // shift MSB to the LEFT by 8 bits
    delay_us(SPI_DELAY);
    registerData |= bcm2835_spi_transfer(0x0F); // MSB, Mid-byte 
    registerData <<= 8;                         // shift MSB , Mid-byte LEFT by 8 bits
    delay_us(SPI_DELAY);
    registerData |= bcm2835_spi_transfer(0x0F); // (MSB, Mid-byte) | LSB
                                                // AD_DRDY should now go HIGH 

    bcm2835_gpio_write(AD_SPI_CS,HIGH);

/*
    bcm2835_spi_end();                          // reset all SPI pins to input mode
*/

    return(registerData);
}


/*    name: ADS1256_ChannelScan
 *    function:  read 8 values of ADS1256 input
 *    parameter: NULL
 *    The return value:  NULL
 *    https://curiousscientist.tech/blog/ads1256-arduino-stm32-sourcecode
 */
void ADS1256_ChannelScan(unsigned nChnl, uint8_t muxCode[], uint8_t rangeCode[], int32_t adScan[])
{
    uint32_t  registerData = 0x00000000; 
    int       chn;

/*
    bcm2835_spi_begin();                   // begin SPI transaction
// ---   SPI.beginTransaction(speed,bitorder,mode)
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST );  //??
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);                //??
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64); // 6.250Mhz on Raspberry Pi 3 

//fprintf(stderr,"spi begin\n");
*/

// channels are written manually, 
// so we save time by switching the SPI.beginTransaction on and off.
// datasheet page 21, figure 19
    for (chn = 0; chn < nChnl; chn++) {
      
      registerData = 0x00000000;           // reset registerData

      while (bcm2835_gpio_lev(AD_DRDY)) { } // wait for DRDY to go low 

      bcm2835_gpio_write(AD_SPI_CS,LOW);   // SPI start

      // Step 1.A - Update MUX  - datasheet page 21 figure 19 
      // Step 1.B - Update PGA  - datasheet page 31 ADCON
      delay_us(SPI_DELAY); 
      bcm2835_spi_transfer(CMD_WREG | REG_MUX); // write from register REG_MUX
      delay_us(SPI_DELAY); 
      bcm2835_spi_transfer(0x01); // Number of Registers to write - 1 = 2-1=1
      delay_us(SPI_DELAY); 
      bcm2835_spi_transfer(muxCode[chn]);            // set the multiplexer
      delay_us(SPI_DELAY);
      bcm2835_spi_transfer( 0x20 | rangeCode[chn] ); // set the PGA
 
      delay_us(5); //??

//    bcm2835_gpio_write(AD_SPI_CS,LOW);   // SPI start //?? 

      bcm2835_spi_transfer(CMD_SYNC);      // Step 2.     
      delay_us(4);                         // t6 delay
      bcm2835_spi_transfer(CMD_WAKEUP_FF);
      delay_us(SPI_DELAY);
      bcm2835_spi_transfer(CMD_RDATA);     // Step 3. 

      delay_us(10);                   // t6 delay (~6.51 us) p.34, fig.30 10us?
     
      // read the sample results to a 24 bit value in 3 8-bit bytes
      // step out the data in this order: MSB | mid-byte | LSB,
      registerData |= bcm2835_spi_transfer(0xFF); // transfer MSB (high 8 bits)
      registerData <<= 8;                  // shift MSB, Low-byte LEFT by 8 bits
      delay_us(SPI_DELAY);
      registerData |= bcm2835_spi_transfer(0xFF); // MSB, Mid-byte 
      registerData <<= 8;                  // shift MSB, Mid-byte LEFT by 8 bits
      delay_us(SPI_DELAY);
      registerData |= bcm2835_spi_transfer(0xFF); // (MSB, Mid-byte) | LSB
                                           // DRDY should now go HIGH 

      // transfer sequence complete, so switch the A/D SPI_CS back to HIGH
      bcm2835_gpio_write(AD_SPI_CS,HIGH);  // SPI stop

      // extend a signed number
      if (registerData & 0x800000)   registerData |= 0xFF000000;   

      // save data to ad_scan array
      adScan[chn] = 2*(int32_t)(registerData);    // *2??
    }
/*
    bcm2835_spi_end();  // reset all SPI pins to input mode   //??
*/
}


/*    name: ADS1256_IntToVolt
 *    function:  convert ADC output integer value to volt [V] 
 *    parameter: value :  output value ( 0 - 0x7fffff )
 *                        reference voltage [V] ( 3.3 or 5.0 )
 *    The return value:  ADC voltage [V]
 * ------------------------------------------------------------------------ */
double ADS1256_IntToVolt(int32_t value , double vref)
{
    return value * 1.0 / ADMAX * vref;
}


/*    name: ADS1256_PrintAllValue
 *    function:  print 8 values of ADS1256 input
 *    parameter: NULL
 *    The return value:  NULL
 * ------------------------------------------------------------------------ */
void ADS1256_PrintAllValue()
{
   uint8_t  chn;
   int32_t ad_scan[NUMCHNL];
   double   volt;

   uint8_t muxCode[8] = { 0B00001000 , 0B00011000 , 0B00101000 , 0B00111000 , 0B01001000 , 0B01011000 , 0B01101000 , 0B01111000 }; 
   uint8_t rangeCode[8] = { 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 };


    ADS1256_ChannelScan(8, muxCode, rangeCode, ad_scan);

    for (chn = 0 ; chn < 8 ; chn++)
    {
        volt = ADS1256_IntToVolt(ad_scan[chn] , 5.0 );
        printf("%d : %8d \t(%f[V])\n", chn , ad_scan[chn], volt );
    }
    printf("\33[8A");   // move the cursor up 8 lines
}


/*    name: ADS1256_PrintAllValueDiff
 *    function:  print 4 values of ADS1256 differential input
 *    parameter: NULL
 *    The return value:  NULL
 * ----------------------------------------------------------------------- */
void ADS1256_PrintAllValueDiff()
{
    int chn;
    int32_t adData;
    for (chn = 0 ; chn < 4 ; chn++)
    {
        adData = ADS1256_GetADC( 2*chn , 2*chn + 1 );
        printf("%d-%d : %8d \t(%10f[V])\n",2*chn, 2*chn + 1 ,adData, ADS1256_IntToVolt(adData,  5.0 ) );
    }
}


//#####################################################################
//  DA Functions


/*
*******************************************************************************
*    name: DAC8532_Write
*    function:  change an output of DAC8532 to target value 
*    parameter:  dac_channel : channel of DAC8532 (0 or 1)
*                        val : output value to DAC8532 ( 0 - 65536 ) ,
*   the DAC8532_VoltToValue function converts the integer val to a voltage
*    The return value:  NULL
*******************************************************************************
*/
void DAC8532_Write( int dac_channel , unsigned int val)
{   

/*
//SPI start
// ---   SPI.beginTransaction(speed,bitorder,mode)
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST );   
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1); 
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64); // 6.250Mhz on Raspberry Pi 3 
*/

    bcm2835_gpio_write(AD_SPI_CS,HIGH);  // ADS1256 SPI end
    bcm2835_gpio_write(DA_SPI_CS,LOW);
    if(dac_channel == 0)
    {
        delay_us(SPI_DELAY);
//      bcm2835_spi_transfer(0x30);
        bcm2835_spi_transfer(0x10);
    }
    else if(dac_channel == 1)
    {
//      bcm2835_spi_transfer(0x34);
        bcm2835_spi_transfer(0x24);
    }

    delay_us(SPI_DELAY);
    bcm2835_spi_transfer( (val & 0xff00) >> 8 );  // send upper 8 bits
    delay_us(SPI_DELAY);
    bcm2835_spi_transfer(  val & 0x00ff );        // send lower 8 bits

    bcm2835_gpio_write(DA_SPI_CS,HIGH);
/*
    bcm2835_spi_end();  // reset all SPI pins to input mode
*/
}


/*
*******************************************************************************
*    name: DAC8532_VoltToValue
*    function:  convert value from volt to 16bit value ( 0 - 65535 )
*    parameter:  volt : target volt [v] ( 0 - 5.0 )
*            volt_ref : reference volt [v] ( 3.3 or 5.0 )
*    The return value:  output value to DAC8532 ( 0 - 65535 )
*******************************************************************************
*/
unsigned int DAC8532_VoltToValue( double volt , double volt_ref)
{
    return ( unsigned int ) ( 65536 * volt / volt_ref );
}


//#####################################################################
//  Private Functions

/*    name: ADS1256_WaitDRDY
 *    function: delay time  wait for automatic calibration
 *    parameter:  NULL
 *    The return value:  NULL
 */
void ADS1256_WaitDRDY(void)
{
    uint32_t i;

    for (i = 0; i < 1000000; i++)
    {
        while(bcm2835_gpio_lev(AD_DRDY)) { /* wait for DRDY to go low */ }
        return;
    }

    perror("ADS1256_WaitDRDY() Time Out ...\r\n");    
}

