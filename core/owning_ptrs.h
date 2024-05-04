// SPDX-License-Identifier: GPL-2.0
// Convenience classes defining owning pointers to C-objects that
// automatically clean up the objects if the pointers go out of
// scope. Based on unique_ptr<>.
// In the future, we should replace these by real destructors.
#ifndef OWNING_PTR_H
#define OWNING_PTR_H

#include <memory>
#include <cstdlib>

struct dive;
struct dive_trip;
struct dive_site;
struct event;

extern "C" void free_dive(struct dive *);
extern "C" void free_trip(struct dive_trip *);

// Classes used to automatically call the appropriate free_*() function for owning pointers that go out of scope.
struct DiveDeleter {
	void operator()(dive *d) { free_dive(d); }
};
struct TripDeleter {
	void operator()(dive_trip *t) { free_trip(t); }
};
struct EventDeleter {
	void operator()(event *ev) { free(ev); }
};

// Owning pointers to dive, dive_trip, dive_site and event objects.
using OwningDivePtr = std::unique_ptr<dive, DiveDeleter>;
using OwningTripPtr = std::unique_ptr<dive_trip, TripDeleter>;
using OwningEventPtr = std::unique_ptr<event, EventDeleter>;

#endif
