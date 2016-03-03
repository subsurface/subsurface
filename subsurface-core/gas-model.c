/* gas-model.c */
/* gas compressibility model */
#include <stdio.h>
#include <stdlib.h>
#include "dive.h"

/*
 * Generic cubic polynomial
 */
static double cubic(double bar, const double coefficient[])
{
	double	x0 = 1.0,
		x1 = bar,
		x2 = x1*x1,
		x3 = x2*x1;

	return	x0 * coefficient[0] +
		x1 * coefficient[1] +
		x2 * coefficient[2] +
		x3 * coefficient[3];
}

/*
 * Cubic least-square coefficients for O2/N2/He based on data from
 *
 *    PERRY’S CHEMICAL ENGINEERS’ HANDBOOK SEVENTH EDITION
 *
 * with the lookup and curve fitting by Lubomir.
 *
 * NOTE! Helium coefficients are a linear mix operation between the
 * 323K and one for 273K isotherms, to make everything be at 300K.
 */
static const double o2_coefficients[4] = {
	+1.00117935180448264158,
	-0.00074149079841471884,
	+0.00000291901111247509,
	-0.00000000162092217461
};

static const double n2_coefficients[4] = {
	+1.00030344355797817778,
	-0.00022528077251905598,
	+0.00000295430303276288,
	-0.00000000210649996337
};

static const double he_coefficients[4] = {
	+1.00000137322788319261,
	+0.000488393024887620665,
	-0.000000054210166760015,
	+0.000000000010908069275
};

static double o2_compressibility_factor(double bar) { return cubic(bar, o2_coefficients); }
static double n2_compressibility_factor(double bar) { return cubic(bar, n2_coefficients); }
static double he_compressibility_factor(double bar) { return cubic(bar, he_coefficients); }

/*
 * We end up using a simple linear mix of the gas-specific functions.
 */
double gas_compressibility_factor(struct gasmix *gas, double bar)
{
	double o2, n2, he;	// Z factors
	double of, nf, hf;	// gas fractions ("partial pressures")

	o2 = o2_compressibility_factor(bar);
	n2 = n2_compressibility_factor(bar);
	he = he_compressibility_factor(bar);

	of = get_o2(gas) / 1000.0;
	hf = get_he(gas) / 1000.0;
	nf = 1.0 - of - nf;

	return o2*of + n2*nf + he*hf;
}
