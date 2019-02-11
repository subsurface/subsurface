// SPDX-License-Identifier: GPL-2.0
// Private definitions for the command-objects

#include <vector>
#include <utility>
#include <QVector>

struct dive;
struct dive_trip;

namespace Command {

// Generally, signals are sent in batches per trip. To avoid writing the same loop
// again and again, this template takes a vector of trip / dive pairs, sorts it
// by trip and then calls a function-object with trip and a QVector of dives in that trip.
// Input parameters:
//	- dives: a vector of trip,dive pairs, which will be sorted and processed in batches by trip.
//	- action: a function object, taking a trip-pointer and a QVector of dives, which will be called for each batch.
template<typename Function>
void processByTrip(std::vector<std::pair<dive_trip *, dive *>> &dives, Function action)
{
	// Use std::tie for lexicographical sorting of trip, then start-time
	std::sort(dives.begin(), dives.end(),
		  [](const std::pair<dive_trip *, dive *> &e1, const std::pair<dive_trip *, dive *> &e2)
		  { return std::tie(e1.first, e1.second->when) < std::tie(e2.first, e2.second->when); });

	// Then, process the dives in batches by trip
	size_t i, j; // Begin and end of batch
	for (i = 0; i < dives.size(); i = j) {
		dive_trip *trip = dives[i].first;
		for (j = i + 1; j < dives.size() && dives[j].first == trip; ++j)
			; // pass
		// Copy dives into a QVector. Some sort of "range_view" would be ideal, but Qt doesn't work this way.
		QVector<dive *> divesInTrip(j - i);
		for (size_t k = i; k < j; ++k)
			divesInTrip[k - i] = dives[k].second;

		// Finally, emit the signal
		action(trip, divesInTrip);
	}
}

// The processByTrip() function above takes a vector if (trip, dive) pairs.
// This overload takes a vector of dives.
template<typename Vector, typename Function>
void processByTrip(Vector &divesIn, Function action)
{
	std::vector<std::pair<dive_trip *, dive *>> dives;
	dives.reserve(divesIn.size());
	for (dive *d: divesIn)
		dives.push_back({ d->divetrip, d });
	processByTrip(dives, action);

}

} // namespace Command
