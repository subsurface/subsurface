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

double redlich_kwong_equation(double t_red, double p_red, double z_init)
{
	return (1.0/(1.0 - 0.08664*p_red/(t_red * z_init)) -
		 0.42748/(sqrt(t_red * t_red * t_red) * ((t_red*z_init/p_red + 0.08664))));
}

/*
 * At high pressures air becomes less compressible, and
 * does not follow the ideal gas law any more.
 */
#define STANDARD_TEMPERATURE 293.0

double gas_compressibility_factor(struct gasmix *gas, double bar)
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
