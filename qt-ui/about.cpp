#include "about.h"
#include "ssrf-version.h"
#include <QDesktopServices>
#include <QUrl>

SubsurfaceAbout::SubsurfaceAbout(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);

	setWindowModality(Qt::ApplicationModal);

	ui.aboutLabel->setText(tr("<span style='font-size: 18pt; font-weight: bold;'>" \
		"Subsurface %1 </span><br><br>" \
		"Multi-platform divelog software<br>" \
		"<span style='font-size: 8pt'>" \
		"Linus Torvalds, Dirk Hohndel, Tomaz Canabrava, and others, 2011-2014" \
		"</span>").arg(VERSION_STRING));
}

void SubsurfaceAbout::on_licenseButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://www.gnu.org/licenses/gpl-2.0.txt"));
}

void SubsurfaceAbout::on_websiteButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://subsurface.hohndel.org"));
}
