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
		-0.00071809207370164567,
		+0.00000281852572807643,
		-0.00000000150290620491
	};
	static const double n2_coefficients[3] = {
		-0.00021926035329221337,
		+0.00000292844845531647,
		-0.00000000207613482075
	};
	static const double he_coefficients[3] = {
		+0.00047961098687979363,
		-0.00000004077670019935,
		+0.00000000000077707035
	};
	double o2, he;
	double x1, x2, x3;
	double Z;

	o2 = get_o2(gas) / 1000.0;
	he = get_he(gas) / 1000.0;

	x1 = bar; x2 = x1*x1; x3 = x2*x1;

	Z = virial_m1(o2_coefficients, x1, x2, x3) * o2 +
	    virial_m1(he_coefficients, x1, x2, x3) * he +
	    virial_m1(n2_coefficients, x1, x2, x3) * (1.0 - o2 - he);

	/*
	 * We add the 1.0 at the very end - the linear mixing of the
	 * three 1.0 terms is still 1.0 regardless of the gas mix.
	 */
	return Z + 1.0;
}
