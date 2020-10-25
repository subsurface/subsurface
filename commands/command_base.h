// SPDX-License-Identifier: GPL-2.0
// Note: this header file is used by the undo-machinery and should not be included elsewhere.

#ifndef COMMAND_BASE_H
#define COMMAND_BASE_H

#include "core/divesite.h"
#include "core/trip.h"
#include "core/dive.h"

#include <QUndoCommand>
#include <QCoreApplication>	// For Q_DECLARE_TR_FUNCTIONS
#include <memory>

// The classes derived from Command::Base represent units-of-work, which can be exectuted / undone
// repeatedly. The command objects are collected in a linear list implemented in the QUndoStack class.
// They contain the information that is necessary to either perform or undo the unit-of-work.
// The usage is:
//	constructor: generate information that is needed for executing the unit-of-work
//	redo(): performs the unit-of-work and generates the information that is needed for undo()
//	undo(): undos the unit-of-work and regenerates the initial information needed in redo()
// The needed information is mostly kept in pointers to dives and/or trips, which have to be added
// or removed.
// For this to work it is crucial that
// 1) Pointers to dives and trips remain valid as long as referencing command-objects exist.
// 2) The dive-table is not resorted, because dives are inserted at given indices.
//
// Thus, if a command deletes a dive or a trip, the actual object must not be deleted. Instead,
// the command object removes pointers to the dive/trip object from the backend and takes ownership.
// To reverse such a deletion, the object is re-injected into the backend and ownership is given up.
// Once ownership of a dive is taken, any reference to it was removed from the backend. Thus,
// subsequent redo()/undo() actions cannot access this object and integrity of the data is ensured.
//
// As an example, consider the following course of events: Dive 1 is renumbered and deleted, dive 2
// is added and renumbered. The undo list looks like this (---> non-owning, ***> owning pointers,
// ===> next item in list)
//
//                                        Undo-List
// +-----------------+     +---------------+     +------------+     +-----------------+
// | Renumber dive 1 |====>| Delete dive 1 |====>| Add dive 2 |====>| Renumber dive 2 |
// +------------------     +---------------+     +------------+     +-----------------+
//          |                      *                   |                     |
//          |      +--------+      *                   |    +--------+       |
//          +----->| Dive 1 |<******                   +--->| Dive 2 |<------+
//                 +--------+                               +--------+
//                                                              ^
//                                    +---------+               *
//                                    | Backend |****************
//                                    +---------+
// Two points of note:
// 1) Every dive is owned by either the backend or exactly one command object.
// 2) All references to dive 1 are *before* the owner "delete dive 2", thus the pointer is always valid.
// 3) References by the backend are *always* owning.
//
// The user undos the last two commands. The situation now looks like this:
//
//
//                  Undo-List                                Redo-List
// +-----------------+     +---------------+     +------------+     +-----------------+
// | Renumber dive 1 |====>| Delete dive 1 |     | Add dive 2 |<====| Renumber dive 2 |
// +------------------     +---------------+     +------------+     +-----------------+
//          |                      *                   *                     |
//          |      +--------+      *                   *    +--------+       |
//          +----->| Dive 1 |<******                   ****>| Dive 2 |<------+
//                 +--------+                               +--------+
//
//                                    +---------+
//                                    | Backend |
//                                    +---------+
// Again:
// 1) Every dive is owned by either the backend (here none) or exactly one command object.
// 2) All references to dive 1 are *before* the owner "delete dive 1", thus the pointer is always valid.
// 3) All references to dive 2 are *after* the owner "add dive 2", thus the pointer is always valid.
//
// The user undos one more command:
//
//       Undo-List                                 Redo-List
// +-----------------+     +---------------+     +------------+     +-----------------+
// | Renumber dive 1 |     | Delete dive 1 |<====| Add dive 2 |<====| Renumber dive 2 |
// +------------------     +---------------+     +------------+     +-----------------+
//          |                      |                   *                     |
//          |      +--------+      |                   *    +--------+       |
//          +----->| Dive 1 |<-----+                   ****>| Dive 2 |<------+
//                 +--------+                               +--------+
//                     ^
//                     *              +---------+
//                     ***************| Backend |
//                                    +---------+
// Same points as above.
// The user now adds a dive 3. The redo list will be deleted:
//
//                                      Undo-List
// +-----------------+                                              +------------+
// | Renumber dive 1 |=============================================>| Add dive 3 |
// +------------------                                              +------------+
//          |                                                             |
//          |      +--------+                               +--------+    |
//          +----->| Dive 1 |                               | Dive 3 |<---+
//                 +--------+                               +--------+
//                     ^                                        ^
//                     *              +---------+               *
//                     ***************| Backend |****************
//                                    +---------+
// Note:
// 1) Dive 2 was deleted with the "add dive 2" command, because that was the owner.
// 2) Dive 1 was not deleted, because it is owned by the backend.
//
// To take ownership of dives/trips, the OnwingDivePtr and OwningTripPtr types are used. These
// are simply derived from std::unique_ptr and therefore use well-established semantics.
// Expressed in C-terms: std::unique_ptr<T> is exactly the same as T* with the following
// twists:
// 1) default-initialized to NULL.
// 2) if it goes out of scope (local scope or containing object destroyed), it does:
//	if (ptr) free_function(ptr);
//    whereby free_function can be configured (defaults to delete ptr).
// 3) assignment between two std::unique_ptr<T> compiles only if the source is reset (to NULL).
//    (hence the name - there's a *unique* owner).
// While this sounds trivial, experience shows that this distinctly simplifies memory-management
// (it's not necessary to manually delete all vector items in the destructur, etc).
// Note that Qt's own implementation (QScoperPointer) is not up to the job, because it doesn't implement
// move-semantics and Qt's containers are incompatible, owing to COW semantics.
//
// Usage:
//	OwningDivePtr dPtr;			// Initialize to null-state: not owning any dive.
//	OwningDivePtr dPtr(dive);		// Take ownership of dive (which is of type struct dive *).
//						// If dPtr goes out of scope, the dive will be freed with free_dive().
//	struct dive *d = dPtr.release();	// Give up ownership of dive. dPtr is reset to null.
//	struct dive *d = d.get();		// Get pointer dive, but don't release ownership.
//	dPtr.reset(dive2);			// Delete currently owned dive with free_dive() and get ownership of dive2.
//	dPtr.reset();				// Delete currently owned dive and reset to null.
//	dPtr2 = dPtr1;				// Fails to compile.
//	dPtr2 = std::move(dPtr1);		// dPtr2 takes ownership, dPtr1 is reset to null.
//	OwningDivePtr fun();
//	dPtr1 = fun();				// Compiles. Simply put: the compiler knows that the result of fun() will
//						// be trashed and therefore can be moved-from.
//	std::vector<OwningDivePtr> v:		// Define an empty vector of owning pointers.
//	v.emplace_back(dive);			// Take ownership of dive and add at end of vector
//						// If the vector goes out of scope, all dives will be freed with free_dive().
//	v.clear(v);				// Reset the vector to zero length. If the elements weren't release()d,
//						// the pointed-to dives are freed with free_dive()

// Qt is making their containers a lot harder to integrate with std::vector
template<typename T>
QVector<T> stdToQt(const std::vector<T> &v)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	return QVector<T>(v.begin(), v.end());
#else
	return QVector<T>::fromStdVector(v);
#endif
}

// We put everything in a namespace, so that we can shorten names without polluting the global namespace
namespace Command {

// Classes used to automatically call the appropriate free_*() function for owning pointers that go out of scope.
struct DiveDeleter {
	void operator()(dive *d) { free_dive(d); }
};
struct TripDeleter {
	void operator()(dive_trip *t) { free_trip(t); }
};
struct DiveSiteDeleter {
	void operator()(dive_site *ds) { free_dive_site(ds); }
};
struct EventDeleter {
	void operator()(event *ev) { free(ev); }
};

// Owning pointers to dive, dive_trip, dive_site and event objects.
typedef std::unique_ptr<dive, DiveDeleter> OwningDivePtr;
typedef std::unique_ptr<dive_trip, TripDeleter> OwningTripPtr;
typedef std::unique_ptr<dive_site, DiveSiteDeleter> OwningDiveSitePtr;
typedef std::unique_ptr<event, EventDeleter> OwningEventPtr;

// This is the base class of all commands.
// It defines the Qt-translation functions
class Base : public QUndoCommand {
	Q_DECLARE_TR_FUNCTIONS(Command)
public:
	// Check whether work is to be done.
	// TODO: replace by setObsolete (>Qt5.9)
	virtual bool workToBeDone() = 0;
};

// Put a command on the undoStack (and take ownership), but test whether there
// is something to be done beforehand by calling the workToBeDone() function.
// If nothing is to be done, the command will be deleted and false is returned.
bool execute(Base *cmd);

// helper function to create more meaningful undo/redo texts (and get the list
// of those texts for the git storage commit message)
QUndoStack *getUndoStack();
QString diveNumberOrDate(struct dive *d);
QString getListOfDives(const std::vector<dive *> &dives);
QString getListOfDives(QVector<struct dive *> dives);

} // namespace Command

#endif // COMMAND_BASE_H
