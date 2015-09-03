#include "about.h"
#include "version.h"
#include <QDesktopServices>
#include <QUrl>
#include <QShortcut>

SubsurfaceAbout::SubsurfaceAbout(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);

	setWindowModality(Qt::ApplicationModal);
	QString versionString(subsurface_git_version());
	QStringList readableVersions = QStringList() << "4.4.96" << "4.5 Beta 1" <<
							"4.4.97" << "4.5 Beta 2" <<
							"4.4.98" << "4.5 Beta 3";
	if (readableVersions.contains(versionString))
		versionString = readableVersions[readableVersions.indexOf(versionString) + 1];

	ui.aboutLabel->setText(tr("<span style='font-size: 18pt; font-weight: bold;'>"
				  "Subsurface %1 </span><br><br>"
				  "Multi-platform divelog software<br>"
				  "<span style='font-size: 8pt'>"
				  "Linus Torvalds, Dirk Hohndel, Tomaz Canabrava, and others, 2011-2015"
				  "</span>").arg(versionString));

	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void SubsurfaceAbout::on_licenseButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://www.gnu.org/licenses/gpl-2.0.txt"));
}

void SubsurfaceAbout::on_websiteButton_clicked()
{
	QDesktopServices::openUrl(QUrl("http://subsurface-divelog.org"));
}
