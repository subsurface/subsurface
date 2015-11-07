#include "windowtitleupdate.h"

WindowTitleUpdate *WindowTitleUpdate::m_instance = NULL;

WindowTitleUpdate::WindowTitleUpdate(QObject *parent) : QObject(parent)
{
	Q_ASSERT_X(m_instance == NULL, "WindowTitleUpdate", "WindowTitleUpdate recreated!");

	m_instance = this;
}

WindowTitleUpdate *WindowTitleUpdate::instance()
{
	return m_instance;
}

WindowTitleUpdate::~WindowTitleUpdate()
{
	m_instance = NULL;
}

void WindowTitleUpdate::emitSignal()
{
	emit updateTitle();
}

extern "C" void updateWindowTitle()
{
	WindowTitleUpdate *wt = WindowTitleUpdate::instance();
	if (wt)
		wt->emitSignal();
}
