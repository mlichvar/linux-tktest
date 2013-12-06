/*
 * Stripped down version of RGR_WeightedRegression from chrony/regress.c
 *
 **********************************************************************
 * Copyright (C) Richard P. Curnow  1997-2003
 * Copyright (C) Miroslav Lichvar  2011
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 **********************************************************************
 */

void
regress
(double *x,                     /* independent variable */
 double *y,                     /* measured data */
 int n,                         /* number of data points */

 /* And now the results */

 double *b0,                    /* estimated y axis intercept */
 double *b1,                    /* estimated slope */
 double *s2

 /* Could add correlation stuff later if required */
)
{
  double P, Q, U, V, W;
  double diff;
  double u, ui;
  int i;

  W = U = 0;
  for (i=0; i<n; i++) {
    U += x[i];
    W += 1.0;
  }

  u = U / W;

  /* Calculate statistics from data */
  P = Q = V = 0.0;
  for (i=0; i<n; i++) {
    ui = x[i] - u;
    P += y[i];
    Q += y[i] * ui;
    V += ui   * ui;
  }

  *b1 = Q / V;
  *b0 = (P / W) - (*b1) * u;

  *s2 = 0.0;
  for (i=0; i<n; i++) {
    diff = y[i] - *b0 - *b1*x[i];
    *s2 += diff*diff;
  }

  *s2 /= (double)(n-2);
}
