/*******************************************************************************
Program HPGdaac.c - High Precision Graphing Data Acquisition And Control
using WaveShare High-Precision AD/DA Board 
https://www.waveshare.com/wiki/High-Precision_AD/DA_Board

    ---- CONFIGURATION  DATA  FILE  FORMAT ----  

Descriptive Title (one line only)
Acquisition Time (duration of test, sec)   :   20.0
Channel scan rate (scans per sec)          :  100.0
Digitization rate 1000, 2000, 3750, 7500   : 1000.0
Number of Channels                         :   3
Channel Positive Pin [0 to 7]              :   0   2   4   3   4   5   6   7
Channel Negative Pin [0 to 7] or -1        :   1   3   5  -1  -1  -1  -1  -1
Voltage Range [0.05 to 5.00] (volts)       : 5.0 5.0 5.0 5.0 5.0 5.0 5.0 5.0
channel information (one line)
Sensor Configuration File Name             : snsrs.cfg
Number of Control Constants                : n
Constant Description 1                     :  
  :      :           :                     :
Constant Description n                     : 
D/A 0 data file name                       : optional e.g., DA-files/chirp0.dat
D/A 1 data file name                       : optional e.g., DA-files/chirp1.dat


    ---- SENSITIVITY  DATA  FILE  FORMAT ----  

Descriptive Title (one line only)
X label : "time, sec"
Y label : "volts"
Channel    Label       Sensitivity  V/Unit  Smoothing    Detrend
===============================================================================
0   "ground acceleration"      0.4  "g"      0.1   2
1   "1st fl. acceleration"     0.4  "g"      0.1   2
2   "2nd fl. acceleration"     0.4  "g"      0.1   2
3   "3rd fl. acceleration"     0.4  "g"      0.1   2

      ---------------

to compile:   make clean; make; sudo make install

to run:   sudo HPGdaac test.cfg data.out
Channel-to-channel skew is ??  micro-sec 
(c) H.P. Gavin, Dept. of Civil Engineering, Duke University, 
2018-08-23 , 2022-01-31 , 2022-10-14, 2023-03-06
*******************************************************************************/

#include <stdio.h>          // standard input/output library routines
#include <string.h>         // standard string handling library
#include <math.h>           // standard mathematics library
#include <signal.h>         // interrupt routines
#include <unistd.h>         // ualarm 

// local libraries ...
#include "../../HPGnumlib/NRutil.h"   // memory allocation routines
#include "../../HPGnumlib/HPGutil.h"  // HPG utility functions
#include "HPGcontrol.h"               // HPG feedback control functions
#include "HPGdaac.h"                  // header file for HPGdaac

//prevent memory swapping ... from bcm2835.h
//nclude <sched.h>
//nclude <sys/mman.h>

#define GRAPHICS     1      /* 1: use realtime graphics ; 0: don't          */

#if GRAPHICS
#include <xcb/xcb.h>
#include "../../HPGxcblib/HPGxcb.h" // HPG xcb gracphics functions
#endif  // GRAPHICS

// external declarations ...
FILE      *fp;              // pointer to the output file of measured DA data

 int32_t  *adData,          // A-to-D data, all in one vector   0 to +16777215
           adScan[NUMCHNL]; // A-to-D data in a single scan     0 to +16777215

uint16_t  *da0Data,         // D-to-A data,  channel 0,    -32767 to    +32768
          *da1Data;         // D-to-A data,  channel 1,    -32767 to    +32768

uint32_t   smpl = 0,        // current sample number 
           scan = 1;        // current scan   number

 uint8_t   muxCode[8]; 
  int      da0=0, da1=0;    // flags indicating D-to-A  

  unsigned chn, nChnl=8,    // number of channels
           scn, nScan,      // total number of channel scans
           spl, nSmpl;      // total number of samples

  float    sr    = 100.0;   // scan rate in scans per second

  float    voltRange = 5.0; // unipolar voltage measurement range
  uint8_t  rangeCode[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // voltage range codes

  struct CHNL chnl[NUMCHNL];     // allocate memory to the array of structures

  struct CTRLCNST ctrlCnst[16];  // allocate memory to the array of structures


#if GRAPHICS
  float    xu=0, xo=0,             // x axis unit, place, offsets
           yu=0, yo=0;             // y axis unit, place, offsets
  static uint32_t CHNL_COLR[8] = {0xffffff, 0xff9205, 0x02ce2a, 0x02c3c4, 0x2201c4, 0xe0ee02, 0xce02c7, 0xf2031d};  // bruel+kjaer
#endif // GRAPHICS

#if CONTROL
float    **Ac, **Bc, **Cc, **Dc;   // L.T.I. state space controller 
#endif // CONTROL


int main (int argc, char *argv[])
{

/* ----------------------- declare variables --------------------------- */

  char     title[MAXL],            // descriptive title of test
           xLabel[MAXL],           // label for x-axis
           yLabel[MAXL],           // label for y-axis
           chnlDesc[MAXL],         // one line of description
           da0fn[MAXL],            // file name for D/A chnl 0
           da1fn[MAXL],            // file name for D/A chnl 1
           sensiFilename[MAXL],    // sensitivity file name
           adDataFilename[MAXL],   // data file name
           ch = ';';               // a character to read

  float    dtime =  20.0,          // number of seconds  spent collecting
           memory;                 // RAM memory allowed for data

#if CONTROL
  float   *xc, *xc1,                 // controller state vector
          *dxcdt,                    // controller state vector derivative
          *y,                        // controller input vector
           vOut = 0.0;               // output voltage from control rule
#endif // CONTROL

  int      integChnl = -1,           // channel to integrate
           diffrChnl = -1;           // channel to differentiate

  uint64_t delta_us = 0,           // microseconds between scans
           pause_us = 0;           // microseceonds from scan stop to scan start

  float    drate = 2000.0;         // digitization rate in samples per second

  time_t   startTime;              // acquisition starting time  

/* ----------------------------------------------------------------------- */

#if CONTROL
// allocate memory for state-variable matrices
  xc    = vector(1,_N);      // allocate controller state vector
  xc1   = vector(1,_N);      // allocate controller state vector
  dxcdt = vector(1,_N);      // allocate controller state derivative
  y     = vector(1,_L);      // allocate  plant output vector
  Ac    = matrix(1,_N,1,_N); // allocate controller dynamics matrix
  Bc    = matrix(1,_N,1,_L); // allocate controller input matrix
  Cc    = matrix(1,_M,1,_N); // allocate controller input matrix
  Dc    = matrix(1,_M,1,_L); // allocate controller input matrix
#endif  // CONTROL 
 
/*prevent memory swapping ... from bcm2835.h
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &sp);
  mlockall(MCL_CURRENT | MCL_FUTURE);
*/

  read_configuration( argc, argv, title, &dtime, &sr, &drate, 
                  &nChnl, muxCode, rangeCode, chnlDesc, chnl, 
                  &da0, &da1, da0fn, da1fn, sensiFilename, ctrlCnst );

  read_sensitivity( argv, sensiFilename, nChnl, 
                   xLabel, yLabel, chnl, &integChnl, &diffrChnl);

  nScan    = (unsigned)(dtime * sr);               // total # channel scans
  nSmpl    = (unsigned)(nChnl * nScan);            // total # A-to-D samples 
  delta_us = (uint64_t)(1.0e6/sr);                 // time step  us
  pause_us = (uint64_t)(1.0e6*(1.0/sr - (double)nChnl/drate)); // time pause us

  memory = 20e6;
  if ( nScan*(nChnl+da0+da1) > memory ) {   // RAM limit on PC, (ha!) 
      errorMsg ("Requested memory exceeds the allowed RAM buffer capacity." );
      fprintf(stderr,"  %.0f bytes were requested",
                             2*(float)nScan*(float)(nChnl+da0+da1) );
      fprintf(stderr,", but only %.0f bytes are available.", 2*memory );
      good_bye ( 0,da0,da1 );
  }

#if CONTROL 
  // define L.T.I. system dynamics
  state_matrices(xc,xc1, dxcdt, y, constants, sr );
#endif  // CONTROL 
 
  if (da0 || CONTROL_DA0) da0Data = u16vector(1,nScan);  // DtoA 0 
  if (da1 || CONTROL_DA1) da1Data = u16vector(1,nScan);  // DtoA 1

  adData = i32vector( 0, nSmpl );             // allocate A-to-D memory
  for (smpl=0; smpl<=nSmpl; smpl++)           // set all samples to 0x0
    adData[smpl] = 0x00000000; 
  smpl = 0;

  // read digital-to-analog data files ---------------------------------
  if (da0) read_da_file ( da0, da0fn, nScan, da0Data );
  if (da1) read_da_file ( da1, da1fn, nScan, da1Data );

  // initialize and reset hardware with  HPADDAlib ---------------------
  initHPADDAboard();
  ADS1256_set_gain (rangeCode[0]);
  drate = ADS1256_SetDigitizationRate(drate);

  fprintf(stderr,"sr= %f delta_us= %llu pause_us= %llu dtime= %f  nChnl= %d  nScan= %u  nSmpl= %u  drate= %8.1f\n", sr, delta_us,pause_us, dtime, nChnl, nScan, nSmpl, drate );


  /* initialize and reset hardware with  WaveShare
    DEV_ModuleInit();
    if(ADS1256_init(0,4) == 1) {
        printf("\r\nEND                  \r\n");
        DEV_ModuleExit();
        exit(0);
    }
   */
  
  // pretest data sample -----------------------------------------------
  pretest_sample_stats( chnl, nChnl, muxCode, 100 );

  // turn on digital outputs -------------------------------------------
  bcm2835_gpio_write(PIN_38, HIGH);
  bcm2835_gpio_write(PIN_40, HIGH);

#if GRAPHICS
  // initialize graphics -----------------------------------------------
  plot_setup ( &xu, &yu, &xo, &yo, 0, dtime, rangeCode, 
               title, xLabel, yLabel, nChnl, chnl );
#endif  // GRAPHICS

//initscr();                               // ncurses
//putchar ('\a');                          // ring when ready 

  bcm2835_gpio_write(PIN_38, LOW);
  bcm2835_gpio_write(PIN_40, LOW);

  int ll = 75-strlen(title);
  color(0); color(1); color(33);
  fprintf (stderr,"________________________________________");
  fprintf (stderr,"________________________________________\n");
  color(33); color(44);
  fprintf (stderr,"  %s %*s  \n", title, ll, " " );
  fprintf (stderr,"  %d scans of %d channels at %6.1f sps and %7.1f cps in %.3f seconds\n", nScan, nChnl, sr, drate, dtime );
  color(0); color(1); color(32);
 
  color(1); color(33);
  fprintf(stderr,"  . . . ready?");  
  color(1); color(32); fprintf(stderr,"  [Y]");
  color(1); color(37); fprintf(stderr,"/");
  color(1); color(31); fprintf(stderr,"n  ");
  color(1); color(37); 
  while ((ch = getchar()) != EOF) {        // get the character
    if ( (ch != ' ') && (ch != '\t' ) ) break;
  }
  fprintf(stderr,"\033[01A");              // move the cursor up     1 
  if ( ch == 'n' ) {
    color(1); color(32);
    fprintf(stderr,"\033[73C");            // move the cursor right 73
    fprintf(stderr," ok.");
    color(1); color(35); fprintf(stderr,"\n");
    good_bye(1,da0,da1);
    exit(0);
  } else {
// https://sites.google.com/a/dee.ufcg.edu.br/rrbrandt/en/docs/ansi/cursor
    fprintf(stderr,"\033[24C");                    // move the cursor right 24 
    color(1); color(32); fprintf(stderr,"  *");
    color(1); color(35); fprintf(stderr," ---------------------------------> ");
    color(1); color(35); fprintf(stderr,"  ok, let's go!  \n"); // start!
  }

  startTime = time(NULL);

  signal( SIGALRM, AD_write_process_DA_plot );
  ualarm( delta_us, delta_us);                     // GO!

  // main data acquisition and control loop
  scan = 0;
  do {
//  bcm2835_gpio_write(PIN_40, HIGH);
//  DEV_Delay_micro(pause_us);
//  AD_write_process_DA_plot(0);
//  ADS1256_PrintAllValue();
//  fprintf(stderr,"x "); fflush(stderr);
//  bcm2835_gpio_write(PIN_40, LOW);
//  fprintf(stderr,"o "); fflush(stderr);
//  fprintf(stderr," . . . scan = %9u  smpl = %9u \n", scan, smpl ); // debug
  } while ( scan < nScan ); 

#if CONTROL
  // memory de-allocation 
  free_vector(xc,1,_N);
  free_vector(xc1,1,_N);
  free_vector(dxcdt,1,_N);
  free_vector(y,1,_L);
  free_matrix(Ac,1,_N,1,_N);      
  free_matrix(Bc,1,_N,1,_L);
  free_matrix(Cc,1,_M,1,_N);
  free_matrix(Dc,1,_M,1,_L);
#endif  // CONTROL

  ualarm( 0 , 0 );                                 // STOP!
  color(1); color(33);
//printf("        . . . test complete . . . \n");  
//printf("        . . . scan = %9u  smpl = %9u \n", scan, smpl ); // debug
//putchar ('\a'); putchar ('\a');            // ring when done

  save_data ( argv, title, nChnl, chnlDesc, chnl, 
              da0Data, da1Data, nScan, drate, sr, dtime, rangeCode, startTime, 
              adDataFilename, sensiFilename);

  enter_esc_to_exit();

  good_bye ( 1,da0,da1 );                  // GOOD BYE !

  exit(0);
}


/* READ_CONFIGURATION -  read start up file, open input/output files    17feb06

    ---- CONFIGURATION  DATA  FILE  FORMAT ----  

Descriptive Title (one line only)
Acquisition Time (duration of test, sec)   :  20.0
Channel scan rate (scans per sec)          : 100.0
Digitization rate 1000, 2000, 3750, 7500   : 1000.0
Number of Channels                         :   3
Channel Positive Pin [0 to 7]              :   0   2   4   3   4   5   6   7
Channel Negative Pin [0 to 7] or -1        :   1   3   5  -1  -1  -1  -1  -1
Voltage Range [0.05 to 5.00] (volts)       : 5.0 5.0 5.0 5.0 5.0 5.0 5.0 5.0
channel information (one line)
Sensor Configuration File Name             : snsrs.cfg
Number of Control Constants                : n
Constant Description 1                     :  
  :      :           :                     :
Constant Description n                     : 
D/A 0 data file name                       : optional e.g., DA-files/chirp0.dat
D/A 1 data file name                       : optional e.g., DA-files/chirp1.dat

------------------------------------------------------------------------------*/
int read_configuration( int argc, char *argv[], char *title,
  float *dtime, float *sr, float *drate, 
  unsigned *nChnl, uint8_t muxCode[], uint8_t *rangeCode, char *chnlDesc,
  struct CHNL  *chnl, int *da0, int *da1, char *da0fn, char *da1fn,
  char *sensiFilename, struct CTRLCNST *ctrlCnst )
{
  char   str[MAXL];
  int8_t posPin[8] = { 0, 1, 2, 3, 4, 5, 6, 7};// pin for positive signal lead
  int8_t negPin[8] = {-1,-1,-1,-1,-1,-1,-1,-1};// pin for negative signal lead
  int    chn, cst;
  float  rangeValue[NUMCHNL] = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0}; 
  int    nConstants = 0;    // number of control constants

  color(1); color(32);

  for (cst=0; cst<16; cst++)  ctrlCnst[cst].val = 0.0;

  if ( argc != 3 ) {
    errorMsg("  usage: HPADDArgc [config file] [data file]  ");
    good_bye ( 0,0,0 );
  }
  if ((fp=fopen(argv[1],"r")) == NULL ) {
    errorMsg("  cannot open configuration data file" );
    color(0); fprintf(stderr,"\n"); color(1); color(33);
    fprintf(stderr,"\t\t\t---- CONFIGURATION DATA  FILE  FORMAT ----\n");
    fprintf(stderr,"Descriptive Title (one line only)\n");
    fprintf(stderr,"Acquisition Time (duration of test, sec)  :   20.0\n");
    fprintf(stderr,"Channel scan rate (scans per sec)         :  100.0\n");
    fprintf(stderr,"Digitization rate 1000, 2000, 3750, 7500  : 1000.0\n");
    fprintf(stderr,"Number of Channels                        :   3\n");
    fprintf(stderr,"Channel Positive Pin [0 to 7]             :   0   2   4   3   4   5   6   7\n");
    fprintf(stderr,"Channel Negative Pin [0 to 7] or -1       :   1   3   5  -1  -1  -1  -1  -1\n");
    fprintf(stderr,"Voltage Range [0.05 to 5.00] (volts)      : 5.0 5.0 5.0 5.0 5.0 5.0 5.0 5.0\n"); 
    fprintf(stderr,"channel information (one line)\n");
    fprintf(stderr,"Sensor Configuration File Name            : snsrs.cfg\n");
    fprintf(stderr,"Number of Control Constants               : n\n");
    fprintf(stderr,"Constant Description 1                    : \n");
    fprintf(stderr,"  :      :           :                    : \n");
    fprintf(stderr,"Constant Description n                    : \n");
    fprintf(stderr,"D/A 0 data file name                      : optional e.g., DA-files/chirp0.dat\n");
    fprintf(stderr,"D/A 1 data file name                      : optional e.g., DA-files/chirp1.dat\n");

    good_bye ( 0,0,0 );
  }

  getLine  ( fp, MAXL, title );
  scanLine ( fp, MAXL, str, ':' );   (void) fscanf ( fp, "%f", dtime );
  scanLine ( fp, MAXL, str, ':' );   (void) fscanf ( fp, "%f", sr );
  scanLine ( fp, MAXL, str, ':' );   (void) fscanf ( fp, "%f", drate );
  scanLine ( fp, MAXL, str, ':' );   (void) fscanf ( fp, "%d", nChnl );
  scanLine ( fp, MAXL, str, ':' ); 
  for (chn = 0; chn < *nChnl; chn++) (void) fscanf ( fp, "%hhd", &posPin[chn] );
  getLine  ( fp, MAXL, chnlDesc );    // clear line
  scanLine ( fp, MAXL, str, ':' ); 
  for (chn = 0; chn < *nChnl; chn++) (void) fscanf ( fp, "%hhd", &negPin[chn] );
  getLine  ( fp, MAXL, chnlDesc );    // clear line
  scanLine ( fp, MAXL, str, ':' ); 
  for (chn = 0; chn < *nChnl; chn++) (void) fscanf (fp, "%f", &rangeValue[chn]);
  getLine  ( fp, MAXL, chnlDesc );    // clear line
  getLine  ( fp, MAXL, chnlDesc );    // read chnlDesc 

  for (chn = 0; chn < *nChnl; chn++) 
    if ( posPin[chn] < 0 || 7 < posPin[chn] ) {
      errorMsg("  read_configuration: posPin out of range.");
      fprintf(stderr,"  posPin must be >= 0 and <= 7");
      good_bye ( 0,0,0 );
    }
  for (chn = 0; chn < *nChnl; chn++) 
    if ( negPin[chn] < -1 || 7 < negPin[chn] ) {
      errorMsg("  read_configuration: negPin out of range.");
      fprintf(stderr,"  negPin must be >= -1 and <= 7");
      good_bye ( 0,0,0 );
    }

  scanLine ( fp, MAXL, str, ':' );  (void) fscanf ( fp, "%s", sensiFilename );

  scanLine ( fp, MAXL, str, ':' );  (void) fscanf ( fp, "%d", &nConstants );
  if ( nConstants > 15 ) {
   errorMsg("  Maximum Number of Control Constants = 15");
   printf("  Entered Number of Control Constants = %d\n",nConstants);
   good_bye ( 0,0,0 );
  }
  for (cst=1; cst <= nConstants; cst++ ) {
    getLine  ( fp, MAXL, str );    // clear line 
    scanLabel ( fp, MAXL, ctrlCnst[cst].label , '"');
    (void) fscanf ( fp, "%f", &ctrlCnst[cst].val );
  }
  if ( nConstants > 0 ) {
    color(0); color(1); color(32);
    printf ("\n");
    printf (" CONSTANT     LABEL         VALUE\n");
    color(0); color(1); color(33);
    printf ("========================================");
    printf ("========================================\n");
    for ( cst = 1; cst <= nConstants; cst++ ) {
      color(1);
      color(32);  printf(" %2d ",cst);
      color(35);  printf("%30s ", ctrlCnst[cst].label );
      color(36);  printf("%12.4e\n", ctrlCnst[cst].val );
      color(0); color(1); color(36);
    }
    printf("\n");
  }

  scanLine ( fp, MAXL, str , ':');
  if ( fscanf(fp,"%s", da0fn ) == 1 )  *da0 = 1;

  scanLine ( fp, MAXL, str , ':');
  if ( fscanf(fp,"%s", da1fn ) == 1 )   *da1 = 1;

  fclose(fp);      /* close the configuration file  */

  if (*drate < 2 * (*nChnl) * (*sr))  *drate = 2 * (*nChnl) * (*sr);

  /*
   set ADS1256 MUX for changing input pins for ADC 
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

  for (chn = 0; chn < *nChnl; chn++) {   // set up mux codes 
    if (negPin[chn] < 0)  negPin[chn] = 8;
    muxCode[chn] = (posPin[chn] << 4) | (negPin[chn]);
    chnl[chn].posPin = posPin[chn];
    chnl[chn].negPin = negPin[chn];
  }

  for (chn = 0; chn < *nChnl; chn++) {  // set up range codes
    rangeCode[chn] = ADS1256_range_code(rangeValue[chn]);
  }

  for (chn = 0; chn < *nChnl; chn++) {  // print mux codes and range codes
    fprintf(stderr," chn %d  mux 0x%2x  range volts %5.3f  range code 0x%2x\n",
                    chn, muxCode[chn], rangeValue[chn], rangeCode[chn] );
  }
  fprintf(stderr,"\n"); 

/*
  if ((fp=fopen(argv[2],"w")) == NULL ) {     // open output file
//  putchar('\a');
    color(0); color(41); 
    fprintf(stderr,"  read_configuration: cannot open output data file '%s'  ", argv[2]);
    errorMsg("  usage: HPGdaac [config file] [data file]  ");
    good_bye ( 0,0,0 );
  } else  fclose(fp);
*/

  return(0);
}


/* READ_SENSITIVITY  - read sensitivity file 

    ---- SENSITIVITY  DATA  FILE  FORMAT ----  

Descriptive Title (one line only)
X label : "time, sec"
Y label : "volts"
integrate channel     :  -1
differentiate channel :  -1 
Channel    Label       Sensitivity  V/Unit  Smoothing    Detrend
===============================================================================
0   "ground acceleration"      0.4  "g"      0.1   2
1   "1st fl. acceleration"     0.4  "g"      0.1   2
2   "2nd fl. acceleration"     0.4  "g"      0.1   2
3   "3rd fl. acceleration"     0.4  "g"      0.1   2

------------------------------------------------------------------------------*/
int read_sensitivity ( char *argv[], char *sensiFilename, unsigned nChnl,
  char *xLabel, char *yLabel, struct CHNL  *chnl,
  int *integChnl, int *diffrChnl )
{
  int   i, chn;
  char  str[MAXL]; 

  if ( ((fp=fopen( sensiFilename,"r" )) == NULL )) {
   fprintf(stderr,"  error:  cannot open sensitivity data file '%s' \n ",
      sensiFilename );
   fprintf(stderr,"\t\t---- SENSITIVITY  DATA  FILE  FORMAT ----\n");
   fprintf(stderr,"  Descriptive Title (one line only) \n");
   fprintf(stderr,"  X label : \"time, sec\"\n");
   fprintf(stderr,"  Y label : \"volts\"\n");
   fprintf(stderr,"  integrate channel     :  -1 \n");
   fprintf(stderr,"  differentiate channel :  -1 \n");
   fprintf(stderr,"  Channel    Label       Sensitivity  V/Unit  Smoothing    Detrend\n");
   fprintf(stderr,"  ===============================================================================\n");
   fprintf(stderr,"  :  \"description label\"       :       \"units\"   [0-1]     [0-5] \n");
   good_bye( 0, 0, 0 );
  }

  for (chn=0; chn<NUMCHNL; chn++)  chnl[chn].bias = (float) ADMID;
  for (chn=0; chn<NUMCHNL; chn++)  chnl[chn].rms  = 0.0;

  (void) getLine ( fp, MAXL, str );
  scanLabel ( fp, MAXL, xLabel, '"' );
  scanLabel ( fp, MAXL, yLabel, '"' );
  scanLine( fp, MAXL, str , ':');
  (void) fscanf ( fp, "%d", integChnl );
  scanLine( fp, MAXL, str , ':');
  (void) fscanf ( fp, "%d", diffrChnl );
  (void) getLine ( fp, MAXL, str );
  (void) getLine ( fp, MAXL, str );
  (void) getLine ( fp, MAXL, str );

  for ( i = 0; i < nChnl; i++) {
    (void) fscanf ( fp,  "%d", &chn );
    scanLabel ( fp, MAXL, chnl[chn].label, '"');
    (void) fscanf ( fp, "%f", &chnl[chn].sensi );
    scanLabel ( fp, MAXL, chnl[chn].units, '"' );
    (void) fscanf ( fp, "%d", &chnl[chn].C );
    (void) fscanf ( fp, "%d", &chnl[chn].D );
    (void) fscanf ( fp, "%f", &chnl[chn].S );

    if (chn < 0 || chn > NUMCHNL ) {
    //putchar('\a');
      color ( 0 );  color ( 41 );
      fprintf(stderr,"error: sensitivity channel %d is out of range\n", chn );
      fprintf(stderr,"Check that the channels in sensitivity data file '%s' ", sensiFilename );
      fprintf(stderr,"correspond to \nthe data acquisition channels in ");
      fprintf(stderr,"configuration file '%s'\n", argv[1] );
      color(0); fprintf(stderr,"\n"); color(1); color(33);
      good_bye( 0,0,0 );
    }
    if ( chnl[chn].S > 1 || chnl[chn].S < 0 ) {
      fprintf(stderr,"error: channel %d ... \n", chn );
      fprintf(stderr," ... smoothing level must be between 0 and 1\n");
      fprintf(stderr,"     0 = no smoothing    1 = full smoothing \n");
      good_bye(0,0,0 );
    }

  }
  fclose(fp);

  return(0);
}


/*
 * READ_DA_FILES  -  read data files for D-to-A conversions    14may96
 * --------------------------------------------------------------------------*/
void read_da_file ( int da, char *dafn, unsigned nScan, uint16_t  *daData)
{
  int        i, head_lines;
  char       ch='a', line[MAXL]; 

  if ( !da ) return;
  
//printf("  DA_00: %d  DA_LO: %d  DA_HI: %d\n", DA_00, DA_LO, DA_HI );

  if ((fp=fopen(dafn,"r")) == NULL ) { // open the DA data file
      //putchar('\a');
      color(0); color(41);
      fprintf(stderr,"  read_configuration: cannot open DA file '%s'  ", dafn);
      good_bye ( 0,0,0 );
  }
//printf(" reading ...  %s \n", dafn );

  head_lines = 0;  
  do { // determine the number of lines in the header
    (void) getLine ( fp, MAXL, line );
    if ( line[0] == '#' || line[0] == '%' ) ++head_lines;
  } while ( line[0] == '#' || line[0] == '%' );
  rewind (fp);

  for (i=1;i<=head_lines;i++)    // scan through the header
    while (( ch = getc(fp)) != '\n') ;

  i=1;

  while ( (fscanf (fp,"%hu", &daData[i])) == 1 ) { // read the data 
    daData[i] += DA_00;          // shift to [0..65535] 
    if ( i > nScan) {
      errorMsg("  read_da_file: DA File  is too long." );
      fprintf(stderr,"  DA File %s hould have one column", dafn);
      fprintf(stderr," of %d numbers or less.  ", nScan);
      fclose(fp);
      good_bye ( 1,da0,da1 );
    }
    i++;
  }
  fclose(fp);

  i--;

  if ( daData[1] != DA_00 || daData[i] != DA_00 ) { // check zero end values
    errorMsg ( "read_da_file: First and last DA values must be ZERO." );
    fprintf(stderr," %s DA[%d] = %hu  ", dafn, 1, daData[1]);
    fprintf(stderr," %s DA[%d] = %hu  ", dafn, i, daData[i]);
    fprintf(stderr,"  ZERO = %d  ", DA_00 );
    good_bye ( 1,da0,da1 );
  }

  while ( i<=nScan) daData[i++] = DA_00; // pad with zero 

  for ( i=1; i<= nScan; i++ ) { // check data range 
    if ( daData[i] < DA_LO || daData[i] > DA_HI ) {
      errorMsg ( "read_da_file: DA out of range" );
      fprintf(stderr," %s DA[%d] = %d ", dafn, i, da0Data[i]);
      fprintf(stderr,"  Limits are: %d <= DA[i] <= %d ", DA_LO, DA_HI);
      good_bye ( 1,da0,da1 );
    }
  }

  return;
}


/*
PRETEST_DATA_STATS
      average of P points of data, at the test stample rate
      rms     of P points of data, at the test stample rate
      This routine does not use Burst mode. If burst mode is enabled,
      ad_init() and ad_range() should be called before and after
      average() is called.  
---------------------------------------------------------------------------*/
void pretest_sample_stats( struct CHNL *chnl, unsigned nChnl, uint8_t muxCode[],  unsigned nScan)
{ 

  unsigned  scn, chn; 
  double    dataValue = 0.0;
  int32_t   adScan[NUMCHNL];

  for ( chn=0; chn < NUMCHNL; chn++ ) chnl[chn].bias = 0.0;
  for ( chn=0; chn < NUMCHNL; chn++ ) chnl[chn].rms  = 0.0;
      
  fprintf(stderr," ADS1256 pretest sample . . . . . . . . . . . . . . . . . . . . .\n"); fflush(stderr);

//printf(" nScan = %u \n", nScan );
  for ( scn = 0; scn < nScan; scn++) {

    ADS1256_ChannelScan(nChnl, muxCode, rangeCode, adScan);

    for ( chn = 0; chn < nChnl;  chn++ ) {
//    printf(" %3d: %6.0f  ", chn, dataValue );  
      dataValue = (double) (adScan[chn]);
      chnl[chn].bias += dataValue; 
      chnl[chn].rms  += dataValue*dataValue;
//    ++spl;
//    delay_us(1000);
    }
//  printf("\n");
  }
  for ( chn = 0; chn < nChnl;  chn++ ) {
    chnl[chn].bias /= (double)(nScan);
    chnl[chn].rms  /= (double)(nScan);
  }
  for ( chn = 0; chn < nChnl;  chn++ ) {
    chnl[chn].rms = sqrt(chnl[chn].rms - chnl[chn].bias*chnl[chn].bias);
  }

  for ( chn = 0; chn < nChnl;  chn++ ) {
    fprintf(stderr,"                        bias[%d] = %8.0f      rms[%d] = %8.0f \n", 
                 chn, chnl[chn].bias, chn, chnl[chn].rms);
  }

  fprintf(stderr,"                        . . . . . . . . . . . . . . . . . . . . .  success \n");

  return;
}


#if 0
/*---------------------------------------------------------------------------
TIME_OUT  -  returns total time & prints time intervals    28may93
----------------------------------------------------------------------------*/
float time_out ( struct time start, struct time stop )
{
  float  start_sec,  // starting time in seconds 
         stop_sec,   // stopping time in seconds
         dt;         // stop_sec - start_sec

/*  start_sec= 3600.0 * start.ti_hour  +  60.0 * start.ti_min
       +  start.ti_sec   +  0.01 * start.ti_hund;
  stop_sec = 3600.0 * stop.ti_hour  +  60.0 * stop.ti_min
       +  stop.ti_sec   +  0.01 * stop.ti_hund;
*/
  dt = stop_sec - start_sec;
/*
  color(0);
  printf("dt: %f\n", dt);
  printf("started at: %02d:%02d:%02d.%02d = %.2f sec\n",
  start.ti_hour, start.ti_min, start.ti_sec, start.ti_hund, start_sec );

  printf("stopped at: %02d:%02d:%02d.%02d = %.2f sec\n",
  stop.ti_hour, stop.ti_min, stop.ti_sec, stop.ti_hund, stop_sec );
*/
  return(dt);
}
#endif  // TIME_OUT


/* AD_WRITE_PROCESS_DA_PLOT -  data aquisition and control handler    02feb22
------------------------------------------------------------------------------*/
void AD_write_process_DA_plot(int signum)
{
  int        chn;             // a data acquisition channel number
//float      data, x;         // a data value

//printf(" . . . scan = %9u  smpl = %9u \n", scan, smpl ); // debug
//bcm2835_gpio_write(PIN_38, HIGH);            // check for time delay 

// AtoD conversions in a scan of the selected AD channels
  ADS1256_ChannelScan(nChnl, muxCode, rangeCode, adScan); // HPADDAlib

//ADS1256_GetAll(firstChnl, lastChnl, adScan); // WaveShare library
//for (chn = firstChnl ; chn <= lastChnl ; chn++)
//  adScan[chn] = ADS1256_ReadDataChn(chn);
//printf("\33[8A");//Move the cursor up 8 lines

  // copy the channel scan data to the adData array
  for ( chn = 0; chn < nChnl; chn++ ) {
    adData[smpl++] = adScan[chn];
//  printf(" data[%2d] = %u \n", chn , data );             // debug
  }

/*
     for(i=0;i<8;i++){
       printf("%2d ... %zu ... %f\r\n",i,adScan[i],adScan[i]*5.0/0x7fffff);
     }
     //printf("\33[8A");//Move the cursor up 8 lines
        
     x = (adScan[0] >> 7)*5.0/0xffff;
     printf(" %f \r\n", x);
     printf(" %f \r\n", 3.3 - x);
     printf("\33[10A");//Move the cursor up 10 lines
*/

// Write_DAC8532( 0, data );  /               // check for time delay

#if CONTROL
  control_rule ( &vOut, xc, y, constants )
#endif

  // send analog output data to the DA channels 
  if (da0 || CONTROL_DA0) DAC8532_Write( 0, da0Data[scan] );
  if (da1 || CONTROL_DA1) DAC8532_Write( 1, da1Data[scan] );

#if GRAPHICS 
  // plot the data scan in real time
  plot_data ( nChnl, sr, scan, adData, xo,yo, xu,yu, rangeCode );
#endif  // GRAPHICS

//for (i=1; i<10000; i++) chn = i*i ;        // real time processing capacity

  bcm2835_gpio_write(PIN_38, LOW);           // check for time delay 
  ++scan;
}


/* 
SAVE_DATA  -  writes signed integers to the data file                28oct96
The 12-bit bipolar data format conversion is:  AD value  voltage    return value
    0x0    -F.S. V        -2048
    0x0800       0  V         0
    0x0FFF    +F.S. V     +2047
The 16-bit bipolar data format conversion is:  AD value  voltage    return value
    0x0    -F.S. V     -32568
    0x8000       0  V    0
    0xFFFF    +F.S. V     +32567
-The 24-bit unipolar data format conversion is:  AD value  voltage    return value
    0x0              0    V            0
    0x8000000     F.S./2  V     
    0xFFFFFFFF     F.S.   V     16777215
------------------------------------------------------------------------------*/
void save_data ( char *argv[], char *title, unsigned nChnl, 
                 char *chnlDesc, struct CHNL *chnl, 
                 uint16_t *da0Data, uint16_t *da1Data,
                 unsigned nScan, float drate, float sr, float dtime,
                 uint8_t *rangeCode, time_t startTime,
                 char *adDataFilename, char *sensiFilename )
{
//char     ch;                     // a character to read
  int      write_shbang = 1,       // #!/bin/bash 
           i=1;
  unsigned scn = 0, chn=0;         // a scan number and a channel number
  char     ch = ';'; 

  double   data_value = 0;
  double   max[NUMCHNL], min[NUMCHNL], // max and min values    
           avg[NUMCHNL], rms[NUMCHNL]; // average and rms values 

  struct tm *start_t = localtime(&startTime);

  sprintf(adDataFilename, "%s.%04d%02d%02d.%02d%02d%02d", argv[2], 
                 start_t->tm_year + 1900,  start_t->tm_mon+1, start_t->tm_mday,
                 start_t->tm_hour, start_t->tm_min, start_t->tm_sec  );

  if ((fp=fopen(adDataFilename,"w")) == NULL ) {     /* open output file */
  //putchar('\a');
    color(0); color(41); 
    fprintf(stderr,"  cannot open output data file '%s'  ", adDataFilename);
    color(0); fprintf(stderr,"\n"); color(1); color(33);
    good_bye ( 0,0,0);
  }

  fprintf(fp, "%% %s", ctime(&startTime) );
  fprintf(fp, "%% %s\n", title );
  fprintf(fp, "%% Data file '%s' created", adDataFilename );
  fprintf(fp, " using configuration '%s' \n", argv[1] );
  fprintf(fp, "%%  %d scans of %d channels at %6.1f sps and %7.1f cps in %.3f seconds\n", nScan, nChnl, sr, drate, dtime );
  fprintf(fp, "%% %s\n", chnlDesc );

  fprintf(fp, "%% voltage ranges \n");
  for (chn=0; chn<nChnl; chn++) {
    if (chn == 0)
      fprintf(fp, "%% %8.4f", ADS1256_range_value(rangeCode[chn]) );
    else
      fprintf(fp, "  %8.4f",  ADS1256_range_value(rangeCode[chn]) );
  } 
  fprintf(fp, "\n");

  fprintf(fp, "%% pre-test sample average \n");
  for (chn = 0; chn < nChnl; chn++) {
    if (chn == 0)
      fprintf(fp,"%% %8.0f", chnl[chn].bias );
    else
      fprintf(fp,"  %8.0f",  chnl[chn].bias );
  }
  fprintf( fp, "\n");
  fprintf(fp, "%% pre-test sample rms \n");
  for (chn = 0; chn < nChnl; chn++) {
    if (chn == 0)
      fprintf(fp,"%% %8.0f", chnl[chn].rms);
    else
      fprintf(fp,"  %8.0f",  chnl[chn].rms);
  }
  fprintf( fp, "\n");
  for (chn = 0; chn < nChnl; chn++) {
      if (chn == 0)
      fprintf ( fp, "%%   chn %2d", chn );
    else
      fprintf ( fp, "    chn %2d",  chn );
  }
  fprintf( fp, "\n");

  for (chn = 0; chn < NUMCHNL; chn++)  
    avg[chn] = rms[chn] = max[chn] = min[chn] = 0.0;

  i = 0;
  for (scn=0; scn<nScan; scn++) {
    for (chn = 0; chn < nChnl; chn++) {

//    data_value = adData[i];
      data_value = (double) (adData[scn*nChnl+chn]);
//    data_value = data_value - chnl[chn].bias;
//    data_value = data_value - ADMID; 
//    if ( scn<=2*(lastChnl-firstChnl) && abs(data_value) > 32000 ) data_value = 0;

      if ( scn <= 20 ) max[chn] = min[chn] += data_value/20;
      avg[chn] += data_value;
      rms[chn] += data_value * data_value;
      if (data_value > max[chn]) max[chn] = data_value; 
      if (data_value < min[chn]) min[chn] = data_value; 

      fprintf(fp,"%10d", (int)(data_value) );
      ++i;
    }

#if CONTROL_DA0
    fprintf(fp, "%10d\t", da0Data[p] - 0x000);
#endif  // CONTROL_DA0

#if CONTROL_DA1
    fprintf(fp, "%10d\t", da1Data[p] - 0x000);
#endif  // CONTROL_DA1

    fprintf(fp, "\n");
    chn = 0;
  } 

  fclose(fp);
  chOwnGrpMod( adDataFilename, 0444 ); 
  
  /* display data statistics to screen */

  for (chn = 0; chn < nChnl; chn++) {
    avg[chn] /= (float) nScan;
    rms[chn] = sqrt ( rms[chn] / (float) nScan - avg[chn]*avg[chn] );

    min[chn] *= ( voltRange / ADMAX );
    max[chn] *= ( voltRange / ADMAX );
    avg[chn] *= ( voltRange / ADMAX );
    rms[chn] *= ( voltRange / ADMAX );
  }

  color(0); color(1); color(33);
  fprintf (stderr,"________________________________________");
  fprintf (stderr,"________________________________________\n");
  color(33); color(44);
  fprintf (stderr,"  %*s  ", 76, strtok(ctime(&startTime), "\n"));
  color(0); color(1); color(32);
  fprintf (stderr,"\n\n");
  fprintf (stderr," CHANNEL     LABEL      ");
  fprintf(stderr,"MIN      MAX      AVG      RMS\n");
  color(0); color(1); color(33);
  fprintf (stderr,"________________________________________");
  fprintf (stderr,"________________________________________\n");
  for (chn = 0; chn < nChnl; chn++) {
    color(1);
    color(32);  fprintf(stderr," %2d ",chn);
    color(35);  fprintf(stderr,"%18s ", chnl[chn].label );
    color(36);  fprintf(stderr,"%8.3f %8.3f %8.3f %8.3f  ",
                        min[chn], max[chn], avg[chn], rms[chn]);
    color(35);  fprintf(stderr,"volts ");
    if ( (min[chn] < -0.95*ADS1256_range_value(rangeCode[chn]) ) ||
         (max[chn] >  0.95*ADS1256_range_value(rangeCode[chn])) ) {
    
//    putchar('\a');
      color(0); color(41);  fprintf(stderr,"CLIPPED!");
      color(0); color(1); color(36);
    }
    fprintf(stderr,"\n");
    fprintf(stderr,"                       ");
    color(36);  fprintf(stderr,"%8.3f %8.3f %8.3f %8.3f  ",
                        min[chn]/chnl[chn].sensi, max[chn]/chnl[chn].sensi,
                        avg[chn]/chnl[chn].sensi, rms[chn]/chnl[chn].sensi);
    color(35);  fprintf(stderr,"%s", chnl[chn].units);
    fprintf(stderr,"\n");
  }

//fflush(stdin);
//ch = getchar();

  color(0); color(1); color(33);
  fprintf (stderr,"________________________________________");
  fprintf (stderr,"________________________________________\n");
  fprintf(stderr,"  data saved to file '%s'   keep file? ", adDataFilename);
  color(1); color(32); fprintf(stderr,"  [Y]");
  color(1); color(37); fprintf(stderr,"/");
  color(1); color(31); fprintf(stderr,"n  ");
  color(1); color(37);
  while((ch = getchar()) != EOF ) {
    if (ch != ' ' && ch != '\t'  && ch != '\n' ) break; 
  }
  fprintf(stderr,"\033[01A");                    // move the cursor up     1 
  fprintf(stderr,"\033[74C");                    // move the cursor right 74 
  if ( ch == 'n' ) {
    color(1); color(35); fprintf(stderr,"ok.");
    color(1); color(35); fprintf(stderr,"\n");
    if (remove(adDataFilename) == 0) {
      fprintf(stderr,"  %s deleted successfully.", adDataFilename);
    } else {
      color(1); color(31); fprintf(stderr,"\n");
      fprintf(stderr,"  Unable to delete %s", adDataFilename);
    }
  } else {
// https://sites.google.com/a/dee.ufcg.edu.br/rrbrandt/en/docs/ansi/cursor
    color(1); color(35); fprintf(stderr,"ok.");
    color(1); color(35); fprintf(stderr,"\n");
    fprintf(stderr,"  %s retained.", adDataFilename);
//  append  scaleall.sh  to speed up the scaling operation 
    write_shbang = 0;
    if ((fp=fopen("scaleall.sh","r")) == NULL )    // scaleall.sh does not exist
      write_shbang = 1;
    if ((fp=fopen("scaleall.sh","a")) == NULL ) {  // open scaleall.sh 
      color(0); color(41);
      fprintf(stderr,"  cannot open scaleing script file '%s'  ","scaleall.sh");
      color(0); fprintf(stderr,"\n"); color(1); color(33);
      good_bye ( 0,0,0 );
    }
    if (write_shbang) fprintf(fp,"#!/bin/bash\n");
//  sprintf(scaled_file,"%s.scl", adDataFilename );
    fprintf(fp,"scale\t%s\t%s\t%s.scl\tresults.dat\n",
                    sensiFilename, adDataFilename, adDataFilename );
    fclose(fp);
    chOwnGrpMod( "scaleall.sh", 0751 ); 

//  append  plotall.sh  to facilitate data plotting
    write_shbang = 0;
    if ((fp=fopen("plotall.sh","r")) == NULL )    // plotall.sh does not exist
      write_shbang = 1;
    if ((fp=fopen("plotall.sh","a")) == NULL ) {  // open plotall.sh 
      color(0); color(41); 
      fprintf(stderr,"  cannot open plotting script file '%s'  ","plotall.sh");
      color(0); fprintf(stderr,"\n"); color(1); color(33);
      good_bye ( 0,0,0 );
    }
    if (write_shbang) {
      fprintf(fp,"# gnuplot script --- load 'plotall.sh'\n");
      fprintf(fp,"set term qt\n");
      fprintf(fp,"set datastyle commentschars '#%%'  \n");
      fprintf(fp,"set xlabel 'sample number' \n");
      fprintf(fp,"set ylabel 'bit value' \n");
    }
    fprintf(fp,"\n\nset autoscale\n");
      fprintf(fp,"set title '%s' \n", adDataFilename );
    fprintf(fp,"plot ");
    for (chn = 0; chn < nChnl; chn++) 
      fprintf(fp," '%s' u %2d w l t '%s', ",adDataFilename, chn, chnl[chn].label );
    fprintf(fp,"\npause(-1)\n");
    fclose(fp);
    chOwnGrpMod( "plotall.sh", 0644 ); 

  }
  color(0); color(1); color(36);
  fprintf(stderr,"\n");

  return;
}


/*
ERROR_MSG  -  displays an error message with the colors and sound  27oct94
------------------------------------------------------------------------------*/
void error_msg ( char err_string[] )
{ 
  putchar('\a');
  color ( 0 );    color ( 41 );
  fprintf(stderr,"  %s  ", err_string );
  color(0); fprintf(stderr,"\n"); color(1); color(33);
  return;
}


/* GOOD_BYE  -  de-allocate memory, close files, change colors, and exit 27oct94
------------------------------------------------------------------------------*/
void good_bye ( int de_alloc, int da0, int da1 )
{
  if ( de_alloc ) {
    free_i32vector ( adData,  0, 1 );
    if ( da0 || CONTROL_DA0)  free_u16vector ( da0Data, 1, 1 );
    if ( da1 || CONTROL_DA1)  free_u16vector ( da1Data, 1, 1 );
  }
/*
  out_w ( DALO0, DA_00 << 4 );
  out_w ( DALO1, DA_00 << 4 );
  dac_init();
*/
  /* fcloseall(); */
// stop, reset, clear, release, and close the hardware 
//DEV_ModuleExit();    // WaveShare library
  closeHPADDAboard();  // HPADDAlib

  color (  0 );
  color ( 36 );
  exit(0);
}


/*
  *lo = DAMIN;
  *hi = DAMAX;
  *DA_00 = DA_00;
  *DA_00 = DA_00;
*/


#if GRAPHICS

/* plot_setup 
 * set up axes scaled to volts and seconds
 * Jesse Hoagg, Fall 2001
 * --------------------------------------------------------------------------*/
void plot_setup ( float *xunit,  float *yunit, float *xo, float *yo,
                  float xmin, float xmax, uint8_t *rangeCode, 
                  char title[], char Xlabel[], char Ylabel[],
                  int8_t nChnl, struct CHNL chnl[] )
{
  float     ymax = +1.0,                //  ADMAX-ADMID, // fractional range
            ymin = -0.0;                //  ADMIN-ADMID; // 0.0;   
  uint8_t   i, chn; 
  int16_t   XB = 25,  YB = 25,          // left and top border for plotting
            dx, dy, xh, yh;             // hatch increments and locations
  char      ht[NUMCHNL];                // hatch text label

  xcb_screen_t        *screen;
  xcb_segment_t        ll[] = { {0,0 , 0,0} };
  xcb_generic_event_t *event;

  uint32_t  gc_mask = 0;                  // graphic context mask
  uint32_t  gc_value[2]  = { 0 , 0 };     // graphic context values
  uint32_t  textClr = 0xecddc6e;          // text and background color
  uint32_t  axisClr[2] = {0x73c4ba, 0x0}; // axis line color  
  // empacher    0xecdc6e
  // bruel+kjaer 0x73c4ba

   if ( chnl[0].negPin >= 0 ) ymin = -1.0;  // differential inputs

// Calculate the pixel size of each x and y unit, 
// the placement of the axis, and size of axis divisions
// The screen size is SCREEN_W by SCREEN_H.
// The origin is in the upper left corner.
  *xunit = (SCREEN_W-XB)/(xmax-xmin);     // display space / value space
//*yunit = (SCREEN_H-YB)/(ymax-ymin);     // display space / value space (bp)
  *yunit = (SCREEN_H-2*YB)/(ymax-ymin);   // display space / value space (up)

// location of the coordinate system in the display space
  *xo = XB + (*xunit)*(-xmin);         
  *yo = YB + (*yunit)*(ymax);
  dx = (int16_t)( MAX ( SCREEN_W - *xo , *xo - XB )/10.0 ); // x-hatch increment
  dy = (int16_t)( MAX ( SCREEN_H - *yo , *yo - YB )/ 5.0 ); // y-hatch increment

/* ------------------------------------------ 
//  check values for setting up the graph
printf("----------------------------\n");
printf(" SCREEN_W = %d \n", SCREEN_W);
printf(" xmin = %f \n", xmin );
printf(" xmax = %f \n", xmax );
printf(" xo = %f \n", *xo);
printf(" xu = %f \n", *xunit);
printf(" dx = %d \n", dx);
printf("----------------------------\n");
printf(" SCREEN_H = %d \n", SCREEN_H);
printf(" ymin = %f \n", ymin );
printf(" ymax = %f \n", ymax );
printf(" yo = %f \n", *yo);
printf(" yu = %f \n", *yunit);
printf(" dy = %d \n", dy);
printf("----------------------------\n");
printf(" title = %s \n", title);
printf(" x-axis label: %s \n", Xlabel );
printf(" y-axis label: %s \n", Ylabel );
printf("----------------------------\n\n");
 ------------------------------------------ */

  // initialize XCB

  // open the connection to the X server 
  connection = xcb_connect (NULL, NULL);

  // get the first screen, window, and foreground 
  screen     = xcb_setup_roots_iterator (xcb_get_setup (connection)).data;
  window     = screen->root;
  foreground = xcb_generate_id (connection);

  // create a graphic context with window foreground and exposure
  gc_mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES ;
  gc_value[0] = 0xFF;
  gc_value[1] = 0x0;
  xcb_create_gc (connection, foreground, window, gc_mask, gc_value);

  // get the window id 
  window = xcb_generate_id (connection);

  // create a graphic context for window background and events 
  gc_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  gc_value[0] = screen->black_pixel;  
  gc_value[1] = XCB_EVENT_MASK_EXPOSURE;

  // create the window 
  xcb_create_window ( connection,                    /* connection      */
                      XCB_COPY_FROM_PARENT,          /* depth           */
                      window,                        /* window Id       */
                      screen->root,                  /* parent window   */
                      0, SCREEN_H/2,                 /* x, y            */
                      SCREEN_W, SCREEN_H,            /* width, height   */
                      10,                            /* border_width    */
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class           */
                      screen->root_visual,           /* visual          */
                      gc_mask, gc_value );           /* mask and values */

  // map the window onto the connection and flush the connection
  xcb_map_window (connection, window);

  gc_mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y; /* window position */
  gc_value[0] = SCREEN_X;
  gc_value[1] = SCREEN_Y;

  xcb_configure_window( connection, window, gc_mask, gc_value );

  xcb_flush (connection);

  // change the title of the window
  xcb_change_property(connection,
        XCB_PROP_MODE_REPLACE,
        window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen(title),
        title);
   xcb_flush (connection);

  // wait (twice ?? (no 20240217) for an "event" and capture it
  event = xcb_wait_for_event (connection);
  event = xcb_wait_for_event (connection);

  // change the graphic context for line colors
  gc_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND ;
  xcb_change_gc  (connection, foreground, gc_mask, axisClr );

  // x-axis coordinate line 
  ll->x1 =  XB;
  ll->y1 = (int16_t)(*yo); 
  ll->x2 =  SCREEN_W; 
  ll->y2 = (int16_t)(*yo);
//printf(" lx = %d %d %d %d \n", (int)(ll->x1), (int)(ll->y1), (int)(ll->x2), (int)(ll->y2) ); // debug
  xcb_poly_segment (connection, window, foreground, 1, ll );

  // y-axis coordinate line 
  ll->x1 = (int16_t)(*xo);
  ll->y1 =  YB; 
  ll->x2 = (int16_t)(*xo);
  ll->y2 =  SCREEN_H-YB;
//printf(" ly = %d %d %d %d \n", (int)(ll->x1), (int)(ll->y1), (int)(ll->x2), (int)(ll->y2) ); //debug
  xcb_poly_segment (connection, window, foreground, 1, ll );

  // hatch marks and numbering on the positive x axis
  for (i=1; i<10; i++) {
    xh = (int16_t)(*xo+i*dx); 
    if (xh > SCREEN_W)  
      break;
    sprintf( ht, "%.1f", (xh-*xo)/(*xunit) );   // x-hatch text
    ll->x1 = xh,
    ll->y1 = (int16_t)(*yo-YB/5)  ; 
    ll->x2 = xh,
    ll->y2 = (int16_t)(*yo+YB/5);
    xcb_poly_segment (connection, window, foreground, 1, ll );
    draw_text (connection, screen, window, xh+3,(int16_t)(*yo+12), textClr,0x0, ht);
  }
  // hatch marks and numbering on the negative x axis
  for (i=1; i<10; i++) {
    xh = (int16_t)(*xo-i*dx); 
    if (xh < XB)  
      break;
    sprintf( ht, "%.1f", (*xo-xh)/(*xunit) );   // x-hatch text
    ll->x1 = xh,
    ll->y1 = (int16_t)(*yo-YB/5)  ; 
    ll->x2 = xh;
    ll->y2 = (int16_t)(*yo+YB/5);
    xcb_poly_segment (connection, window, foreground, 1, ll );
    draw_text (connection, screen, window, xh+3,(int16_t)(*yo+12), textClr,0x0, ht);
  }
  // hatch marks and numbering on the positive y axis
  for (i=1; i<5; i++) {
    yh = (int16_t)(*yo-i*dy); 
    if (yh < YB)  
      break;
    sprintf( ht, "%4.1f", (*yo-yh)/(*yunit) );   // y-hatch text
    ll->x1 = (int16_t)(*xo-XB/5)  ; 
    ll->y1 = yh,
    ll->x2 = (int16_t)(*xo+XB/5);
    ll->y2 = yh,
    xcb_poly_segment (connection, window, foreground, 1, ll );
    draw_text (connection, screen, window, (int16_t)(*xo-XB), yh-3, textClr,0x0,ht);
  }
  // hatch marks and numbering on the negative y axis
  for (i=1; i<5; i++) {
    yh = (int16_t)(*yo+i*dy); 
    if (yh > SCREEN_H)  
      break;
    sprintf( ht, "%4.1f", (*yo-yh)/(*yunit) );   // y-hatch text
    ll->x1 = (int16_t)(*xo-XB/5)  ; 
    ll->y1 = yh,
    ll->x2 = (int16_t)(*xo+XB/5);
    ll->y2 = yh,
    xcb_poly_segment (connection, window, foreground, 1, ll );
    draw_text (connection, screen, window, (int16_t)(*xo-XB), yh-3, textClr,0x0,ht);
  }

// draw title and axis labels 
//draw_text (connection,screen,window, SCREEN_W/4, YB/2, textClr,0x0, title );
  draw_text (connection,screen,window, (int)(0.95*SCREEN_W),*yo+YB/2, textClr,0x0, Xlabel);
  draw_text (connection,screen,window, (int)(XB/2),(int)(YB-6), textClr,0x0, "full scale" );

  //  Display the channel labels in color of the lines.
  gc_mask     = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
  for ( chn = 0; chn < nChnl; chn++ ) {
//  sprintf(chnl[chn].label, "channel %2d", chn); // debug
    gc_value[0] = CHNL_COLR[chn];
    xcb_change_gc  (connection, foreground, gc_mask, gc_value);
    draw_text (connection, screen, window,
            SCREEN_W-100, YB+15*(chn+1), CHNL_COLR[chn],0x0, chnl[chn].label );
  }

  fprintf(stderr," plot set up  . . . . . . . . . . . . . . . . . . . . . . . . . .  success \n"); 
  return;
}


/*
 * plot_data - plot a scan of data points scaled to volts and seconds
 * -------------------------------------------------------------------------*/
void plot_data ( int8_t nChnl, float sr, unsigned scn,  int32_t adData[],
                 float xo, float yo, float xu, float yu, uint8_t *rangeCode)
{
  uint8_t        chn;
  xcb_point_t    p = {0,0};
  
  uint32_t gc_mask     = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
  uint32_t gc_value[2] = { 0x0 , 0x0 };

//delay_us(10);
  for (chn = 0; chn < nChnl; chn++) {

    p.x = (int16_t)(xo+xu*(scn/sr));
    p.y = (int16_t)(yo-yu*((adData[scn*nChnl+chn-1]))/ADMAX);
//  p.y = (int16_t)(yo-yu*((ADMID)*voltRange/ADMAX));   // bi-polar AtoD

//  fprintf(stderr," ... p.x = %5" PRId16 "  p.y = %5" PRId16 "\n", p.x, p.y );

    gc_value[0] = CHNL_COLR[chn];
    xcb_change_gc  (connection, foreground, gc_mask, gc_value);
    xcb_poly_point (connection, XCB_COORD_MODE_ORIGIN, window, foreground,1,&p);

/*
    if ( scn>0 ) line( display,
    (int)(y_axis+xu*((scn-1)/sr-xoff)) ,
    (int)(x_axis-yu*((adData[scn*nChnl+i-nChnl]-0x8000)*voltRange/32567-yoff)) ,
    (int)(y_axis+xu*(scn/sr-xoff)) , 
    (int)(x_axis-yu*((adData[scn*nChnl+i]-0x8000)*voltRange/32576-yoff)) , i );
*/
  }             
  xcb_flush (connection);      // wait for all N_smpls samples before flushing
  return;
}


/*
 * plot_bffr_data - plot a scan of data points scaled to volts and seconds
 * -------------------------------------------------------------------------*/
void plot_bffr_data ( int8_t firstChnl, int8_t lastChnl, unsigned nChnl,
                     float sr, unsigned nBffr, unsigned spl,  uint16_t adData[],
                     float xo, float yo, float xu, float yu, uint8_t *rangeCode)
{
  unsigned       firstScn = (unsigned)((float)(spl-nBffr)/nChnl),
                 lastScn  = (unsigned)((float)spl/nChnl),
                 scn, chn,sp;
  xcb_point_t    p = {0,0};
  
  uint32_t gc_mask     = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
  uint32_t gc_value[2] = { 0x0 , 0x0 };

//delay_us(10);
  sp=0;
  for (scn = firstScn; scn <= lastScn; scn++) {
    p.x = (int16_t)(xo+xu*(scn/sr));
    for (chn = firstChnl; chn <= lastChnl; chn++) {

//    p.y = (int16_t)(yo-yu*((adData[scn*nChnl+chn])*voltRange/ADMAX));
//    p.y = (int16_t)(yo-yu*((ADMID)*voltRange/ADMAX));   // bi-polar AtoD
      p.y = (int16_t)(yo-yu*((float)(adData[spl-nBffr+sp])/ADMID-1.0)); // bi-polar AtoD

//rintf(stderr," scn= %4d chn= %4d sp=%4d  data=%d ... p.x= %6d  p.y= %6d \n", scn, chn,  spl-nBffr+sp, adData[scn*nChnl+chn], p.x, p.y );

      gc_value[0] = CHNL_COLR[chn];
      xcb_change_gc  (connection, foreground, gc_mask, gc_value);
      xcb_poly_point (connection, XCB_COORD_MODE_ORIGIN, window, foreground,1,&p);
      ++sp;

/*
      if ( scan>0 ) line( display,
      (int)(y_axis+xu*((scan-1)/sr-xoff)) ,
      (int)(x_axis-yu*((adData[scan*nChnl+i-nChnl]-0x8000)*voltRange/32567-yoff)) ,
      (int)(y_axis+xu*(scan/sr-xoff)) , 
      (int)(x_axis-yu*((adData[scan*nChnl+i]-0x8000)*voltRange/32576-yoff)) , i );
*/
    }
    if ( sp >= nBffr )  break; 
  }             
  xcb_flush (connection);      // wait for all N_smpls samples before flushing
  return;
}

#endif  // GRAPHICS


#undef  MAXL
