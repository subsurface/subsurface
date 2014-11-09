#include <string.h>

#include "dive.h"
#include "divelist.h"
#include "file.h"


// Convert bytes into an INT
#define array_uint16_le(p) ((unsigned int) (p)[0] \
							+ ((p)[1]<<8) )
#define array_uint32_le(p) ((unsigned int) (p)[0] \
							+ ((p)[1]<<8) + ((p)[2]<<16) \
							+ ((p)[3]<<24))

#if __WIN32__
static char *strndup (const char *s, size_t n)
{
	char *cpy;
	size_t len = strlen(s);
	if (n < len)
		len = n;
	if ((cpy = malloc(len + 1)) != NULL) {
		cpy[len] = '\0';
		memcpy(cpy, s, len);
	}
	return cpy;
}
#endif

struct lv_event {
	time_t time;
	struct pressure {
		int sensor;
		int mbar;
	} pressure;
};

static int handle_event_ver2(int code, const unsigned char *ps, unsigned int ps_ptr, struct lv_event *event)
{
	// Skip 4 bytes
	return 4;
}


static int handle_event_ver3(int code, const unsigned char *ps, unsigned int ps_ptr, struct lv_event *event)
{
	int skip = 4;

	switch (code) {
	case 0x0002:	//	Unknown
	case 0x0004:	//	Unknown
		skip = 4;
		break;
	case 0x0005:	// Unknown
		skip = 6;
		break;
	case 0x0007:	// Gas
		// 4 byte time
		// 1 byte O2, 1 bye He
		skip = 6;
		break;
	case 0x0008:
		// 4 byte time
		// 2 byte gas set point 2
		skip = 6;
		break;
	case 0x000f:
		// Tank pressure
		event->time = array_uint32_le(ps + ps_ptr);
		event->pressure.sensor = 0; //array_uint16_le(ps + ps_ptr + 4);
		event->pressure.mbar = array_uint16_le(ps + ps_ptr + 6) * 10; // cb->mb
		// 1 byte PSR
		// 1 byte ST
		skip = 10;
		break;
	case 0x0010:
		skip = 26;
		break;
	case 0x0015:	// Unknown
		skip = 2;
		break;
	default:
		skip = 4;
		break;
	}

	return skip;
}

static void parse_dives (int log_version, const unsigned char *buf, unsigned int buf_size)
{
	unsigned int ptr = 0;
	unsigned char model;

	struct dive *dive;
	struct divecomputer *dc;
	struct sample *sample;

	while (ptr < buf_size) {
		dive = alloc_dive();
		dc = &dive->dc;

		// Model 0=Xen, 1,2=Xeo, 4=Lynx, other=Liquivision
		model = *(buf + ptr);
		switch (model) {
		case 0:
			dc->model = "Xen";
			break;
		case 1:
		case 2:
			dc->model = "Xeo";
			break;
		case 4:
			dc->model = "Lynx";
			break;
		default:
			dc->model = "Liquivision";
			break;
		}
		ptr++;

		// Dive location, assemble Location and Place
		unsigned int len, place_len;
		len = array_uint32_le(buf + ptr);
		ptr += 4;
		place_len = array_uint32_le(buf + ptr + len);

		if (len && place_len) {
			dive->location = malloc(len + place_len + 4);
			memset(dive->location, 0, len + place_len + 4);
			memcpy(dive->location, buf + ptr, len);
			memcpy(dive->location + len, ", ", 2);
			memcpy(dive->location + len + 2, buf + ptr + len + 4, place_len);
		} else if (len) {
			dive->location = strndup(buf + ptr, len);
		} else if (place_len) {
			dive->location = strndup(buf + ptr + len + 4, place_len);
		}

		ptr += len + 4 + place_len;

		// Dive comment
		len = array_uint32_le(buf + ptr);
		ptr += 4;

		// Blank notes are better than the default text
		if (len && strncmp(buf + ptr, "Comment ...", 11)) {
			dive->notes = strndup(buf + ptr, len);
		}
		ptr += len;

		dive->id = array_uint32_le(buf + ptr);
		ptr += 4;

		dive->number = array_uint16_le(buf + ptr) + 1;
		ptr += 2;

		dive->duration.seconds = array_uint32_le(buf + ptr);	// seconds
		ptr += 4;

		dive->maxdepth.mm = array_uint16_le(buf + ptr) * 10;	// cm->mm
		ptr += 2;

		dive->meandepth.mm = array_uint16_le(buf + ptr) * 10;	// cm->mm
		ptr += 2;

		dive->when = array_uint32_le(buf + ptr);
		ptr += 4;

		//unsigned int end_time = array_uint32_le(buf + ptr);
		ptr += 4;

		//unsigned int sit = array_uint32_le(buf + ptr);
		ptr += 4;
		//if (sit == 0xffffffff) {
		//}

		dive->surface_pressure.mbar = array_uint16_le(buf + ptr);		// ???
		ptr += 2;

		//unsigned int rep_dive = array_uint16_le(buf + ptr);
		ptr += 2;

		dive->mintemp.mkelvin =  C_to_mkelvin((float)array_uint16_le(buf + ptr)/10);// C->mK
		ptr += 2;

		dive->maxtemp.mkelvin =  C_to_mkelvin((float)array_uint16_le(buf + ptr)/10);// C->mK
		ptr += 2;

		dive->salinity = *(buf + ptr);	// ???
		ptr += 1;

		unsigned int sample_count = array_uint32_le(buf + ptr);
		ptr += 4;

		// Sample interval
		unsigned char sample_interval;
		sample_interval = 1;

		unsigned char intervals[6] = {1,2,5,10,30,60};
		if (*(buf + ptr) < 6)
			sample_interval = intervals[*(buf + ptr)];
		ptr += 1;

		float start_cns = 0;
		unsigned char dive_mode = 0, algorithm = 0;
		if (array_uint32_le(buf + ptr) != sample_count) {
			// Xeo, with CNS and OTU
			start_cns = *(float *) (buf + ptr);
			ptr += 4;
			dive->cns = *(float *) (buf + ptr);	// end cns
			ptr += 4;
			dive->otu = *(float *) (buf + ptr);
			ptr += 4;
			dive_mode = *(buf + ptr++);	// 0=Deco, 1=Gauge, 2=None
			algorithm = *(buf + ptr++);	// 0=ZH-L16C+GF
			sample_count = array_uint32_le(buf + ptr);
		}
		// we aren't using the start_cns, dive_mode, and algorithm, yet
		(void)start_cns;
		(void)dive_mode;
		(void)algorithm;

		ptr += 4;

		// Parse dive samples
		const unsigned char *ds = buf + ptr;
		const unsigned char *ts = buf + ptr + sample_count * 2 + 4;
		const unsigned char *ps = buf + ptr + sample_count * 4 + 4;
		unsigned int ps_count = array_uint32_le(ps);
		ps += 4;

		// Bump ptr
		ptr += sample_count * 4 + 4;

		// Handle events
		unsigned int ps_ptr;
		ps_ptr = 0;

		unsigned int event_code, d = 0, e;
		struct lv_event event;

		// Loop through events
		for (e = 0; e < ps_count; e++) {
			// Get event
			event_code = array_uint16_le(ps + ps_ptr);
			ps_ptr += 2;

			if (log_version == 3) {
				ps_ptr += handle_event_ver3(event_code, ps, ps_ptr, &event);
				if (event_code != 0xf)
					continue;	// ignore all by pressure sensor event
			} else {	// version 2
				ps_ptr += handle_event_ver2(event_code, ps, ps_ptr, &event);
				continue;		// ignore all events
			}
			int sample_time, last_time;
			int depth_mm, last_depth, temp_mk, last_temp;

			while (true) {
				sample = prepare_sample(dc);

				// Get sample times
				sample_time = d * sample_interval;
				depth_mm = array_uint16_le(ds + d * 2) * 10; // cm->mm
				temp_mk = C_to_mkelvin((float)array_uint16_le(ts + d * 2) / 10); // dC->mK
				last_time = (d ? (d - 1) * sample_interval : 0);

				if (d == sample_count) {
					// We still have events to record
					sample->time.seconds = event.time;
					sample->depth.mm = array_uint16_le(ds + (d - 1) * 2) * 10; // cm->mm
					sample->temperature.mkelvin = C_to_mkelvin((float) array_uint16_le(ts + (d - 1) * 2) / 10); // dC->mK
					sample->sensor = event.pressure.sensor;
					sample->cylinderpressure.mbar = event.pressure.mbar;
					finish_sample(dc);

					break;
				} else if (event.time > sample_time) {
					// Record sample and loop
					sample->time.seconds = sample_time;
					sample->depth.mm = depth_mm;
					sample->temperature.mkelvin = temp_mk;
					finish_sample(dc);
					d++;

					continue;
				} else if (event.time == sample_time) {
					sample->time.seconds = sample_time;
					sample->depth.mm = depth_mm;
					sample->temperature.mkelvin = temp_mk;
					sample->sensor = event.pressure.sensor;
					sample->cylinderpressure.mbar = event.pressure.mbar;
					finish_sample(dc);

					break;
				} else {	// Event is prior to sample
					sample->time.seconds = event.time;
					sample->sensor = event.pressure.sensor;
					sample->cylinderpressure.mbar = event.pressure.mbar;
					if (last_time == sample_time) {
						sample->depth.mm = depth_mm;
						sample->temperature.mkelvin = temp_mk;
					} else {
						// Extrapolate
						last_depth = array_uint16_le(ds + (d - 1) * 2) * 10; // cm->mm
						last_temp = C_to_mkelvin((float) array_uint16_le(ts + (d - 1) * 2) / 10); // dC->mK
						sample->depth.mm = last_depth + (depth_mm - last_depth)
							* (event.time - last_time) / sample_interval;
						sample->temperature.mkelvin = last_temp + (temp_mk - last_temp)
							* (event.time - last_time) / sample_interval;
					}
					finish_sample(dc);

					break;
				}
			} // while (true);
		} // for each event sample

		// record trailing depth samples
		for ( ;d < sample_count; d++) {
			sample = prepare_sample(dc);
			sample->time.seconds = d * sample_interval;

			sample->depth.mm = array_uint16_le(ds + d * 2) * 10; // cm->mm
			sample->temperature.mkelvin =
				C_to_mkelvin((float)array_uint16_le(ts + d * 2) / 10);
			finish_sample(dc);
		}

		if (log_version == 3 && model == 4) {
			// Advance to begin of next dive
			switch (array_uint16_le(ps + ps_ptr)) {
			case 0x0000:
				ps_ptr += 5;
				break;
			case 0x0100:
				ps_ptr += 7;
				break;
			case 0x0200:
				ps_ptr += 9;
				break;
			case 0x0300:
				ps_ptr += 11;
				break;
			case 0x0b0b:
				ps_ptr += 27;
				break;
			}

			while (*(ps + ps_ptr) != 0x04)
				ps_ptr++;
		}

		// End dive
		dive->downloaded = true;
		record_dive(dive);
		mark_divelist_changed(true);

		// Advance ptr for next dive
		ptr += ps_ptr + 4;
	} // while

	//DEBUG save_dives("/tmp/test.xml");
}

int try_to_open_liquivision(const char *filename, struct memblock *mem)
{
	const unsigned char *buf = mem->buffer;
	unsigned int buf_size = mem->size;
	unsigned int ptr;
	int log_version;

	// Get name
	unsigned int len = array_uint32_le(buf);
	// Ignore name
	ptr = 4 + len;

	unsigned int dive_count = array_uint32_le(buf + ptr);
	if (dive_count == 0xffffffff) {
		// File version 3.0
		log_version = 3;
		ptr += 6;
		dive_count = array_uint32_le(buf + ptr);
	} else {
		log_version = 2;
	}
	ptr += 4;

	parse_dives(log_version, buf + ptr, buf_size - ptr);

	return 1;
}
