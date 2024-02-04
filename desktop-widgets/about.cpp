// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/about.h"
#include "core/version.h"
#include <QDesktopServices>
#include <QUrl>
#include <QShortcut>

SubsurfaceAbout::SubsurfaceAbout(QWidget *parent) : QDialog(parent, QFlag(0))
{
	ui.setupUi(this);

	setWindowModality(Qt::ApplicationModal);
	QString versionString(subsurface_canonical_version());

	ui.aboutLabel->setText(tr("<span style='font-size: 18pt; font-weight: bold;'>"
				  "Subsurface %1 </span><br><br>"
				  "Multi-platform divelog software<br>"
				  "<span style='font-size: 8pt'>"
				  "Linus Torvalds, Dirk Hohndel, Tomaz Canabrava, Berthold Stoeger, and others, 2011-2024"
				  "</span>").arg(versionString));

	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void SubsurfaceAbout::on_licenseButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://www.gnu.org/licenses/gpl-2.0.txt"));
}

void SubsurfaceAbout::on_contributeButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://subsurface-divelog.org/contribute/"));
}

void SubsurfaceAbout::on_websiteButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://subsurface-divelog.org/"));
}

void SubsurfaceAbout::on_creditButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://subsurface-divelog.org/credits/"));
}
