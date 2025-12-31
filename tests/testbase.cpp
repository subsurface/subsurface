// SPDX-License-Identifier: GPL-2.0
#include "core/errorhelper.h"
#include "testbase.h"
#include <memory>

TestBase *TestBase::instance()
{
	static std::unique_ptr<TestBase> instance;
	if (!instance)
		instance = std::make_unique<TestBase>();

	return instance.get();
}

static void failOnError(std::string error)
{
	TestBase::instance()->failOnError(error);
}

void TestBase::failOnError(const std::string& error)
{
	if (TestBase::skipErrors)
		report_info("Skipping error: %s", error.c_str());
	else
		QFAIL(("Error reported: " + error).c_str());
}

void TestBase::setSkipErrors(bool enabled)
{
	skipErrors = enabled;
}

void TestBase::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);

	set_error_cb(&::failOnError);
}

bool TestBase::skipErrors = false;
