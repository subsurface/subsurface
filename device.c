#include <string.h>
#include "dive.h"
#include "device.h"

struct divecomputer* fake_dc(struct divecomputer* dc)
{
	static struct sample fake[4];
	static struct divecomputer fakedc;
	static bool initialized = 0;
	if (!initialized){
		fakedc = (*dc);
		fakedc.sample = fake;
		fakedc.samples = 4;

		/* The dive has no samples, so create a few fake ones.  This assumes an
		ascent/descent rate of 9 m/min, which is just below the limit for FAST. */
		int duration = dc->duration.seconds;
		int maxdepth = dc->maxdepth.mm;
		int asc_desc_time = dc->maxdepth.mm*60/9000;
		if (asc_desc_time * 2 >= duration)
			asc_desc_time = duration / 2;
		fake[1].time.seconds = asc_desc_time;
		fake[1].depth.mm = maxdepth;
		fake[2].time.seconds = duration - asc_desc_time;
		fake[2].depth.mm = maxdepth;
		fake[3].time.seconds = duration * 1.00;
		fakedc.events = dc->events;
	}
	return &fakedc;
}

