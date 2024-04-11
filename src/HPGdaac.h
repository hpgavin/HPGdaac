/*
 * ==========================================================================
 *
 *       Filename:  HPADDAgc.h
 *
 *    Description:  header file for HPADDAgc.c
 *
 *         Author:  Henri P. Gavin 
 *
 * ==========================================================================
 */

#ifndef _HPADDAGC_H_
#define _HPADDAGC_H_
#include "HPADDAlib.h"        // High-Performance AD/DA library files

// coordinates of screen position and screen dimentions
#define SCREEN_X   870   
#define SCREEN_Y    10
#define SCREEN_W  1205
#define SCREEN_H   590

#define MAXL       256   /* maximum line length allowed for title & sens */

#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

  struct CHNL {        // channel information
         char label[MAXL];     // channel description
         int8_t posPin;        // chanel positive pin
         int8_t negPin;        // chanel negative pin
         float range;          // full scale measurement range, volts
         float sensi;          // channel sensitivity, Volts/unit
         char units[MAXL];     // sensitivity units
         double bias;          // mean of pre-test data  (decimal integer value)
         double rms;           // rms  of pre-test data  (decimal integer value)
         int   C;              // declip type
         int   D;              // detrending types 
         float S;              // smoothing level           
      };

  extern struct CHNL chnl[NUMCHNL];

  struct CTRLCNST {     // control constants for feedback control
         char label[MAXL];     // control constant label
         float val;            // control constant value
      };

  extern struct CTRLCNST ctrlCnst[16];

/* read configuration file, open output data file  */
int read_configuration( int argc, 
                        char *argv[], 
                        char *title,
                        float *dtime, 
                        float *sr, 
                        float *drate, 
                        unsigned *nChnl, 
                        uint8_t muxCode[], 
                        uint8_t *rangeCode, 
                        char chnlDesc[],
                        struct CHNL chnl[], 
                        int *da0, 
                        int *da1, 
                        char *da0fn, 
                        char *da1fn,
                        char *sensiFilename, 
                        struct CTRLCNST *ctrlCnst ); 

/* read sensor sensitivity data file        */
int read_sensitivity ( char *argv[], 
                       char *sensiFilename, 
                       unsigned nChnl, 
                       char *xLabel, 
                       char *yLabel, 
                       struct CHNL  *chnl, 
                       int *integChnl, 
                       int *diffrChnl );

/* read digital-to-analog (D-to-A) data file       */
void read_da_file ( int da, 
                    char *dafn, 
                    unsigned nScan, 
                    uint16_t  *daData );

/* sample statistics of  nScan pretest data scans */
void pretest_sample_stats( struct CHNL *chnl, 
                           unsigned nChnl, 
                           uint8_t muxCode[],
                           unsigned nScan );


/* collect an observation, store it, process it, output controls, plot */
void AD_write_process_DA_plot(int signum);

/* write analog-to-digital (A-to-D) data files       */
void save_data ( char *argv[], 
                 char *title, 
                 unsigned nChnl, 
                 char *testDesc, 
                 struct CHNL *chnl, 
                 uint16_t *da0data, 
                 uint16_t *da1data, 
                 unsigned nScan,
                 float drate, 
                 float csr, 
                 float dtime, 
                 uint8_t *rangeCode, 
                 time_t startTime, 
                 char *adDataFilename, 
                 char *sensiFilename); 


/* err message with sound and color */
void error_msg ( char errString[] );

/* close files, color, exit     */
void good_bye ( int de_alloc, 
                int da0, 
                int da1 ); 


// float time_out ( struct time start, struct time stop );

/* compute state matrices      */
void state_matrices ( float *xc, 
                      float *xc1, 
                      float *dxcdt, 
                      float *y,
                      float *constants, 
                      float csr );

/* continuous time to discrete time */
void c2d ( float **Ac, 
           float **Bc, 
           float csr );

/* compute state derivative    */
void state_deriv ( float *y, 
                   float *xc, 
                   float *dxcdt );

/* 4th Order Runge-Kutta Method */
void rk4 ( float *y, 
           float *dydx, 
           int n, 
           float h, 
           float vi, 
           float *yout,
	   void (*derivs)() );

/* simulate discrete time systm */
void DT_sim ( float *xc1, 
              float *xc, 
              float *y );

/* control rule                 */
void control_rule ( float V_out, 
                    float *xc1, 
                    float *y, 
                    float *constants );

/* sign of a value */
float   sgn( float x );


/* set up a plot screen */
void plot_setup ( float    *xunit,
                  float    *yunit,
                  float    *xo,
                  float    *yo,
                  float     xmin,
                  float     xmax,
                  uint8_t  *rangeCode,
                  char      title[],
                  char      xLabel[],
                  char      yLabel[],
                  int8_t    nChnl,
                  struct CHNL chnl[] );

/* plot a scan of data points */
void plot_data ( int8_t     nChnl,
                 float      sr,
                 unsigned   scan,
                 int32_t    adData[],
                 float      xo,
                 float      yo,
                 float      xu,
                 float      yu,
                 uint8_t   *rangeCode );

/* plot a buffer of data points */
void plot_bffr_data ( int8_t     firstChnl,
                      int8_t     lastChnl,
                      unsigned   nChnl,
                      float      sr,
                      unsigned   nBffr,
                      unsigned   spl,
                      uint16_t   adData[],
                      float      xo,
                      float      yo,
                      float      xu,
                      float      yu,
                      uint8_t   *rangeCode );


#endif // _HPADDAGC_H_ 
