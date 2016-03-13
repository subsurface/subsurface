/* gas-model.c */
/* gas compressibility model */
#include <stdio.h>
#include <stdlib.h>
#include "dive.h"

/* "Virial minus one" - the virial cubic form without the initial 1.0 */
#define virial_m1(C, x1, x2, x3) (C[0]*x1+C[1]*x2+C[2]*x3)

/*
 * Cubic virial least-square coefficients for O2/N2/He based on data from
 *
 *    PERRY’S CHEMICAL ENGINEERS’ HANDBOOK SEVENTH EDITION
 *
 * with the lookup and curve fitting by Lubomir.
 *
 * The "virial" form of the compression factor polynomial is
 *
 *      Z = 1.0 + C[0]*P + C[1]*P^2 + C[2]*P^3 ...
 *
 * and these tables do not contain the initial 1.0 term.
 *
 * NOTE! Helium coefficients are a linear mix operation between the
 * 323K and one for 273K isotherms, to make everything be at 300K.
 */
double gas_compressibility_factor(struct gasmix *gas, double bar)
{
	static const double o2_coefficients[3] = {
		-7.18092073703e-04,
		+2.81852572808e-06,
		-1.50290620492e-09
	};
	static const double n2_coefficients[3] = {
		-2.19260353292e-04,
		+2.92844845532e-06,
		-2.07613482075e-09
	};
	static const double he_coefficients[3] = {
		+4.87320026468e-04,
		-8.83632921053e-08,
		+5.33304543646e-11
	};
	int o2, he;
	double x1, x2, x3;
	double Z;

	o2 = get_o2(gas);
	he = get_he(gas);

	x1 = bar; x2 = x1*x1; x3 = x2*x1;

	Z = virial_m1(o2_coefficients, x1, x2, x3) * o2 +
	    virial_m1(he_coefficients, x1, x2, x3) * he +
	    virial_m1(n2_coefficients, x1, x2, x3) * (1000 - o2 - he);

	/*
	 * We add the 1.0 at the very end - the linear mixing of the
	 * three 1.0 terms is still 1.0 regardless of the gas mix.
	 *
	 * The * 0.001 is because we did the linear mixing using the
	 * raw permille gas values.
	 */
	return Z * 0.001 + 1.0;
}
