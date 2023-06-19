/*******************************************************************************
HPGcontrol.c
routines for real time control
*******************************************************************************/

#include <stdio.h>          // standard input/output library routines
#include <string.h>         // standard string handling library
#include <math.h>           // standard mathematics library

// local libraries .. .
#include "HPGcontrol.h"          // header for feedback control functions
#include "../../HPGnumlib/NRutil.h" // memory allocation routines

#if CONTROL

/* STATE_MATRICES - the matrix coefficients which define the integrator 17jul01
---------------------------------------------------------------------------*/
void state_matrices ( float *xc, float *xc1, float *dxcdt, float *y,
  float *constants, float sr )
{
  float  gainP = 0.0,  /* proportional error feedback gain  */
    gainI = 0.0,  /* integral error feedback gain    */
    gainD = 0.0,  /* derivitive error feedback gain  */
    gainF = 0.0;  /* proportional command feedforward gain */
/*
  gainP  = constants[1];
  gainI  = constants[2];
  gainD  = constants[3];
  gainF  = constants[4];
*/

/*   Jesse's integrator state-varriable constants */
  float  r1=0.15846e6,  r2=0.16931e6,  r3=1.40752e6,
    c1=8.53433e-6, c2=6.90523e-6, c3=1.56000e-4, c4=4.28669e-9,
    scale = 0.03;

/* Julie's integrator state-varriable constants */
/*  float  r1=0.21715e6,  r2=0.15388e6,  r3=3.63108e6,
    c1=8.71817e-5, c2=5.41742e-6, c3=1.56000e-4, c4=1.91548e-9,
    scale=0.0276;
*/

  int  i, j;     /* counter for initilizing state matrix */

  void  c2d();    /* convert continuous time to discrete time */

  for (i=1; i<=_N; i++) {  /* zero all values for L.T.I. system  */
    xc[i] = xc1[i] = dxcdt[i] = 0.0;
   for (j=1; j<=_N; j++)  Ac[i][j] = 0.0;
    for (j=1; j<=_L; j++)  Bc[i][j] = 0.0;
  }
  for (i=1; i<=_M; i++) {  
    y[i] = 0.0;
   for (j=1; j<=_N; j++)  Cc[i][j] = 0.0;
    for (j=1; j<=_L; j++)  Dc[i][j] = 0.0;
  }

/* 
  Ac[1][2] = 1.0,
  Ac[2][1] = -1.0/(r1*r2*c1*c2),
  Ac[2][2] = -(r1+r2)/(r1*r2*c1),
  Ac[3][2] = c3/c4,
  Ac[3][3] = -1.0/(r3*c4);
  
  Bc[1][1] = 0.0,
  Bc[2][1] = scale*1.0/(r1*r2*c1*c2),
  Bc[3][1] = 0.0;
*/
  Ac[1][2] =  1.0;
  Ac[2][1] = -9.8135e4;
  Ac[2][2] = -4.9097e2;

  Bc[1][1] = 0.0;
  Bc[2][1] = 1.0;

  Cc[1][2] = gainD * 1.1747e5;
//  Cc[1][3] = gainI * 0;

  Dc[1][1] = gainP;
  Dc[1][2] = gainF;

  c2d(Ac,Bc,sr); /* continuous time to discrete time  */

  color(1);
  for (i=1; i<=_N; i++) {  /* zero all values for L.T.I. system  */
    color(32); printf(" Ac:");
    color(36);
   for (j=1; j<=_N; j++)  printf(" %10.3e ", Ac[i][j] );
    color(32); printf(" Bc:");
    color(36);
    for (j=1; j<=_L; j++)  printf(" %10.3e ", Bc[i][j] );
    printf("\n");
  }
  for (i=1; i<=_M; i++) {  
    color(32); printf(" Cc:");
    color(36);
   for (j=1; j<=_N; j++)  printf(" %10.3e ", Cc[i][j] );
    color(32); printf(" Dc:");
    color(36);
    for (j=1; j<=_L; j++)  printf(" %10.3e ", Dc[i][j] );
    printf("\n");
  }
  printf("\n");
  
  return;
}


/* DT_SIM - evaluate xc at time (t+1) 
  xc(t+1) = Ac * xc(t) + Bc * y(t)   ... for the Discrete Time Dynamics
-----------------------------------------------------------------------------*/
void DT_sim ( float *xc1, float *xc, float *y )
{
  int  i, j;

  for (i=1; i<=_N; i++) {
    xc1[i] = 0.0;
    for (j=1; j<=_N; j++)  xc1[i] += Ac[i][j] * xc[j];
    for (j=1; j<=_L; j++)  xc1[i] += Bc[i][j] * y[j];
  }
  return;
}


/* CONTROL_RULE - calculate control rule for analog output signal     24july01
-----------------------------------------------------------------------------*/
void control_rule ( float *V_out, float *xc, float *y, float *constants )
{
  float  threshold = 100.0,  /* threshold to supress chatter */
    K_iso = 6.6,    /* isolation stiffness  kN/cm  */
    alpha = 0.2, beta = 0.2, gamma = 0.2,  /* ctrl cnstnts */
    sgn();      /* sign of a float    */

  float  accel = 0.0, force = 0.0, displ = 0.0;

  int  i, j;

  /* proportional error feedback for pseudo-negative stiffness */

  K_iso       = constants[1];
  alpha       = constants[2];
  beta  = constants[3];
  gamma       = constants[4];
  threshold   = constants[5];
  accel       = y[1];
  displ       = y[2];
  force      = y[3];

  if ( force*displ < 0.0 ) {
    if ( fabs(force) < threshold )    // connect
  //    *V_out = threshold_2;
      *V_out = gamma*fabs(displ);
    else     // control
  //    *V_out += beta*( K_iso*displ + force ) * sgn(displ);
      *V_out = beta*fabs(displ);
    if ( *V_out < 0.0 )
      *V_out = 0.0;
    if ( *V_out > 1.0 )
      *V_out = 1.0;
  } else {
//    if ( fabs(force) > threshold )    // dis-connect
      *V_out = alpha;     
//    else    // don't re-connect
//      *V_out = 0.0;
  }

//  if ( force*displ > 0.0 ) {
//    if ( fabs(force) > threshold )
//      *V_out = -alpha * fabs(force);
//    else
//      *V_out = 0.0;
//  }
//  else  
//    *V_out += beta*( force + K_iso*displ ) * sgn(displ);

#if 0  /* state variable controller output equation u = Cc*xc + Dc*y */

  *V_out = 0.0;
  for (i=1; i<=_N; i++)
    if ( fabs(xc[i]) < 10000.0 )
  *V_out += Cc[1][i]*xc[i];  // _M = 1
  for (i=1; i<=_L; i++)  *V_out += Dc[1][i]* y[i];  // _M = 1
#endif  // state variable controller output equation

  if (*V_out < -9.0) *V_out = -9.0;
  if (*V_out >  9.0) *V_out =  9.0;

  return;
}

/*// process the data in real time for feedback control 
#if CONTROL
  
    y[1] = ad_data[smpl-stop_chnl+RT_CHN_1] - chnl[RT_CHN_1].bias; 
    y[1] *=  range / ADMAX / sensi[RT_CHN_1];

    y[2] = ad_data[smpl-stop_chnl+RT_CHN_2] - chnl[RT_CHN_2].bias; 
    y[2] *=  range / ADMAX / sensi[RT_CHN_2];

    y[3] = ad_data[smpl-stop_chnl+RT_CHN_3] - chnl[RT_CHN_3].bias; 
    y[3] *=  range / ADMAX / sensi[RT_CHN_3];

#if 0
    // S.L. table control stuff 
    ref_cmnd = ((float)da0_data[t]-2048.8)/2048.0 * max_displ;

    y[1] -= ref_cmnd;    // position error 
    y[2] = ref_cmnd;     // reference command 
#endif  // S.L. table control stuff

    // state_deriv ( y, xc, dxcdt );
    // rk4 ( xc, dxcdt, _N, 1.0/sr, y, xc, state_deriv);
    
    DT_sim( xc1, xc, y );
    control_rule( &V_out, xc1, y, constants );

//    V_out = -5.0 + t*10.0/N_scans;    // for testing
//    V_out = ref_cmnd;      // for testing

#if CONTROL_DA0
    da0_data[t] = (int)((V_out / 10.1 + 1.0) * 2047.0 ); // 12 bit
#endif  // CONTROL DA0 
#if CONTROL_DA1
    da1_data[t] = (int)((V_out / 10.1 + 1.0) * 2047.0 ); // 12 bit
#endif  // CONTROL_DA1

    xc = xc1;    // copy contents of xc1 into xc 

#endif  // CONTROL 
*/



/* STATE_DERIV  -  calculate the state derivitive of a LTI system    17jul01
    dx/dt = A x + B y
-----------------------------------------------------------------------------*/
void state_deriv ( float *y, float *xc, float *dxcdt )
{
  int i,j;

  for (i=1; i<=_N; i++) {
    dxcdt[i] = 0.0;
    for (j=1; j<=_N; j++)  dxcdt[i] += Ac[i][j]*xc[j];
       for (j=1; j<=_L; j++)  dxcdt[i] += Bc[i][j]*y[j];
  }

  return;
}


/* RK4  -  4th Order Runge-Kutta, ``Numerical Recipes In C,'' 17julyo1
---------------------------------------------------------------------------*/
void rk4 ( float *y, float *dydx, int n, float h, float vi, float *yout, 
  void (*derivs)() )
{
  int  i;
  float  hh, h6, *dym, *dyt, *yt;

  dym = vector(1,n);
  dyt = vector(1,n);
  yt  = vector(1,n);

  hh = h/2.0;
  h6 = h/6.0;

  for(i=1;i<=n;i++)
    yt[i] = y[i]+hh*dydx[i];

  (*derivs)(vi,yt,dyt);
  for(i=1;i<=n;i++)
    yt[i] = y[i]+hh*dyt[i];

  (*derivs)(vi,yt,dym);
  for(i=1;i<=n;i++) {
    yt[i] = y[i]+h*dym[i];
    dym[i] += dyt[i];
  }

  (*derivs)(vi,yt,dyt);
  for(i=1;i<=n;i++)
    yout[i] = y[i]+h6*(dydx[i]+dyt[i]+2.0*dym[i]);

  free_vector(yt,1,n);
  free_vector(dyt,1,n);
  free_vector(dym,1,n);
  return;
}


/* C2D - convert Continuous time dynamics to Discrete time dynamics
-----------------------------------------------------------------------------*/
// c2d
/*
  Ak = A;
  k_factorial = 1;

  max_iter = 100;
  tolerance = 1e-6;

 for k = 1:max_iter

    expmA = expmA + Ak/k_factorial;

    if all( abs(Ak(find(expmA~=0))) ./ abs(expmA(find(expmA~=0)))/k_factorial < tolerance )
  break;
    end
    
    Ak = Ak*A;
    k_factorial = k_factorial*(k+1);
 end
  

    7 June 2007
---------------------------------------------------------------------------*/


/* SGN - return the sign of x    28 Oct 03
-----------------------------------------------------------------------------*/
float  sgn( float x )
{
  if (x > 0.0)  return  1.0 ;
  if (x < 0.0)  return -1.0 ;
  if (x == 0.0)  return  0.0 ;
  return(0.0);
}

#endif
