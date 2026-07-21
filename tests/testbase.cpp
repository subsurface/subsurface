// SPDX-License-Identifier: GPL-2.0
#include "testparse.h"
#include "core/errorhelper.h"

// We don't link the undo-code into tests, so we have to provide
// a placeholder for Command::changesMade(), which is needed by
// the git code.
namespace Command {
	std::string changesMade() { return {}; }
}

static void failOnError(std::string error)
{
       QFAIL(("Error reported: " + error).c_str()); 
}

void TestBase::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);

	set_error_cb(&::failOnError);
}
