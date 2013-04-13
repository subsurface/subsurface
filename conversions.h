/*
 * conversions.h
 *
 * Helpers to convert between different units
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

void convert_volume_pressure(int ml, int mbar, double *v, double *p);
int convert_pressure(int mbar, double *p);

#ifdef __cplusplus
}
#endif
