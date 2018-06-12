// SPDX-License-Identifier: GPL-2.0
#include "qmlprefs.h"

QMLPrefs *QMLPrefs::m_instance = NULL;


QMLPrefs::QMLPrefs()
{
	if (!m_instance)
		m_instance = this;
}

QMLPrefs::~QMLPrefs()
{
	m_instance = NULL;
}

QMLPrefs *QMLPrefs::instance()
{
	return m_instance;
}
