/*
 * ==========================================================================
 *
 *       Filename:  HPGcontrol.h
 *
 *    Description:  header file for HPGcontrol.c
 *
 *         Author:  Henri P. Gavin 
 *
 * ==========================================================================
 */

#define CONTROL      0   /* perform realtime control computation         */
#define CONTROL_DA0  0   /* online processing for analog output 0        */
#define CONTROL_DA1  0   /* online processing for analog output 1        */
#define RT_CHN_1     1   /* an input channel for realtime processing     */
#define RT_CHN_2     2   /* an input channel for realtime processing     */
#define RT_CHN_3     3   /* an input channel for realtime processing     */

#define _N           2   /* number of dynamic states in controller       */
#define _L           2   /* number of controller inputs                  */
#define _M           1   /* number of controller outputs                 */

#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

#if CONTROL

/* compute state matrices      */
void state_matrices ( float *xc, float *xc1, float *dxcdt, float *y,
        float *constants, float csr );

/* continuous time to discrete time */
void c2d ( float **Ac, float **Bc, float csr );

/* compute state derivative    */
void state_deriv ( float *y, float *xc, float *dxcdt );

/* 4th Order Runge-Kutta Method */
void rk4 ( float *y, float *dydx, int n, float h, float vi, float *yout,
	void (*derivs)() );

/* simulate discrete time systm */
void DT_sim ( float *xc1, float *xc, float *y );

/* control rule                 */
void control_rule ( float V_out, float *xc1, float *y, float *constants );

/* sign of a value */
float   sgn( float x );

#endif
