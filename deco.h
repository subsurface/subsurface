#ifndef DECO_H
#define DECO_H

#ifdef __cplusplus
extern "C" {
#endif

extern double tolerated_by_tissue[];
extern double buehlmann_N2_t_halflife[];
extern double tissue_inertgas_saturation[16];
extern double buehlmann_inertgas_a[16], buehlmann_inertgas_b[16];
extern double gf_low_pressure_this_dive;


#ifdef __cplusplus
}
#endif

#endif // DECO_H
