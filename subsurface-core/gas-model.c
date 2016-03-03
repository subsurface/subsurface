/* gas-model.c */
/* gas compressibility model */
#include <stdio.h>
#include <stdlib.h>
#include "dive.h"

/*
 * This gives an interative solution of hte Redlich-Kwong equation for the compressibility factor
 * according to https://en.wikipedia.org/wiki/Redlich–Kwong_equation_of_state
 * in terms of the reduced temperature T/T_crit and pressure p/p_crit.
 *
 * Iterate this three times for good results in our pressur range.
 *
 */

static double redlich_kwong_equation(double t_red, double p_red, double z_init)
{
	return (1.0/(1.0 - 0.08664*p_red/(t_red * z_init)) -
		 0.42748/(sqrt(t_red * t_red * t_red) * ((t_red*z_init/p_red + 0.08664))));
}

/*
 * At high pressures air becomes less compressible, and
 * does not follow the ideal gas law any more.
 */
#define STANDARD_TEMPERATURE 293.0

static double redlich_kwong_compressibility_factor(struct gasmix *gas, double bar)
{
	/* Critical points according to https://en.wikipedia.org/wiki/Critical_point_(thermodynamics) */

	double tcn2 = 126.2;
	double tco2 = 154.6;
	double tche = 5.19;

	double pcn2 = 33.9;
	double pco2 = 50.5;
	double pche = 2.27;

	double tc, pc;

	tc = (tco2 * get_o2(gas) + tche * get_he(gas) + tcn2 * (1000 - get_o2(gas) - get_he(gas))) / 1000.0;
	pc = (pco2 * get_o2(gas) + pche * get_he(gas) + pcn2 * (1000 - get_o2(gas) - get_he(gas))) / 1000.0;

	return (redlich_kwong_equation(STANDARD_TEMPERATURE/tc, bar/pc,
				       redlich_kwong_equation(STANDARD_TEMPERATURE/tc, bar/pc,
							      redlich_kwong_equation(STANDARD_TEMPERATURE/tc, bar/pc,1.0))));
}

/*
 * Generic quintic polynomial
 */
static double quintic(double bar, const double coefficient[])
{
	double	x0 = 1.0,
		x1 = bar,
		x2 = x1*x1,
		x3 = x2*x1,
		x4 = x2*x2,
		x5 = x2*x3;

	return	x0 * coefficient[0] +
		x1 * coefficient[1] +
		x2 * coefficient[2] +
		x3 * coefficient[3] +
		x4 * coefficient[4] +
		x5 * coefficient[5];
}

/*
 * These are the quintic coefficients by Lubomir I. Ivanov that have
 * been optimized for the least-square error to the air
 * compressibility factor table (at 300K) taken from Wikipedia:
 *
 * bar  z_factor
 * ---  ------
 *   1: 0.9999
 *   5: 0.9987
 *  10: 0.9974
 *  20: 0.9950
 *  40: 0.9917
 *  60: 0.9901
 *  80: 0.9903
 * 100: 0.9930
 * 150: 1.0074
 * 200: 1.0326
 * 250: 1.0669
 * 300: 1.1089
 * 400: 1.2073
 * 500: 1.3163
 */
static const double air_coefficients[6] = {
	+1.0002556612420115,
	-0.0003115084635183305,
	+0.00000227808965401253,
	+1.91596422989e-9,
	-8.78421542e-12,
	+6.77746e-15
};

/*
 * Quintic least-square coefficients for O2/N2/He based on tables at
 *
 *    http://ww.baue.org/library/zfactor_table.php
 *
 * converted to bar and also done by Lubomir.
 */
static const double o2_coefficients[6] = {
	+1.0002231211532653,
	-0.0007471497056767194,
	+0.00000200444854807816,
	+2.91501995188e-9,
	-4.48294663e-12,
	-6.11529e-15
};

static const double n2_coefficients[6] = {
	+1.0001898816185364,
	-0.00030793319362077315,
	+0.00000327557417347714,
	-1.93872574476e-9,
	-2.7732353e-12,
	-2.8921e-16
};

static const double he_coefficients[6] = {
	+0.9998700261301693,
	+0.0005452130351730479,
	-2.7853712233619e-7,
	+5.5935404211e-10,
	-1.35114572e-12,
	+2.00476e-15
};

static double air_compressibility_factor(double bar) { return quintic(bar, air_coefficients); }
static double o2_compressibility_factor(double bar) { return quintic(bar, o2_coefficients); }
static double n2_compressibility_factor(double bar) { return quintic(bar, n2_coefficients); }
static double he_compressibility_factor(double bar) { return quintic(bar, he_coefficients); }

/*
 * We end up using specialized functions for known gases, because
 * we have special tables for them.
 *
 * For air we use our known-good table. For other mixes we use a
 * linear interpolation of the Z factors of the individual gases.
 */
double gas_compressibility_factor(struct gasmix *gas, double bar)
{
	double o2, n2, he;	// Z factors
	double of, nf, hf;	// gas fractions ("partial pressures")

	if (gasmix_is_air(gas))
		return air_compressibility_factor(bar);

	o2 = o2_compressibility_factor(bar);
	n2 = n2_compressibility_factor(bar);
	he = he_compressibility_factor(bar);

	of = gas->o2.permille / 1000.0;
	hf = gas->he.permille / 1000.0;
	nf = 1.0 - of - nf;

	return o2*of + n2*nf + he*hf;
}
