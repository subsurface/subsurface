#ifndef GASPRESSURES_H
#define GASPRESSURES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * simple structure to track the beginning and end tank pressure as
 * well as the integral of depth over time spent while we have no
 * pressure reading from the tank */
typedef struct pr_track_struct pr_track_t;
struct pr_track_struct {
	unsigned int start;
	unsigned int end;
	unsigned int t_start;
	unsigned int t_end;
	unsigned int pressure_time;
	pr_track_t *next;
};

typedef struct pr_interpolate_struct pr_interpolate_t;
struct pr_interpolate_struct {
	unsigned int start;
	unsigned int end;
	unsigned int pressure_time;
	unsigned int acc_pressure_time;
};

enum interpolation_strategy {SAC, TIME, CONSTANT};

#ifdef __cplusplus
}
#endif
#endif // GASPRESSURES_H
