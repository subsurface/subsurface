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
 * This is a quintic formula by Lubomir I. Ivanov that has
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
static double air_compressibility_factor(double bar)
{
	double	x0 = 1.0,
		x1 = bar,
		x2 = x1*x1,
		x3 = x2*x1,
		x4 = x2*x2,
		x5 = x2*x3;

	return  + x0 * 1.0002556612420115
		- x1 * 0.0003115084635183305
		+ x2 * 0.00000227808965401253
		+ x3 * 1.91596422989e-9
		- x4 * 8.78421542e-12
		+ x5 * 6.77746e-15;
}

/*
 * We end up using specialized functions for known gases, because
 * we have special tables for them.
 *
 * For now, let's do just air.
 *
 * We have other tables for other gases, see for example:
 *
 *   http://ww.baue.org/library/zfactor_table.php
 *
 * and then we have the Redlich-Kwong function, but that seems
 * to be almost too generic, and not specific enough to the very
 * particular pressure and temperature ranges we care about..
 */
double gas_compressibility_factor(struct gasmix *gas, double bar)
{
#if 1
	return air_compressibility_factor(bar);
#else
	/* Fall back on generic function */
	return redlich_kwong_compressibility_factor(gas, bar);
#endif
}
