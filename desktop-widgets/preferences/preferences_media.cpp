// SPDX-License-Identifier: GPL-2.0
#include "preferences_media.h"
#include "ui_preferences_media.h"
#include "core/settings/qPrefMedia.h"
#include "core/qthelper.h"
#include "qt-models/models.h"

#include <QApplication>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QFileDialog>
#include <QProcess>

PreferencesMedia::PreferencesMedia() : AbstractPreferencesWidget(tr("Media"), QIcon(":preferences-media-icon"), 6)
{
	ui = new Ui::PreferencesMedia();
	ui->setupUi(this);
}

PreferencesMedia::~PreferencesMedia()
{
	delete ui;
}

void PreferencesMedia::checkFfmpegExecutable()
{
	QString s = ui->ffmpegExecutable->text().trimmed();

	// If the user didn't provide a string they probably didn't intend to run ffmeg,
	// so let's not give an error message.
	if (s.isEmpty())
		return;

	// Try to execute ffmpeg. But wait at most 2 sec for startup and execution
	// so that the UI doesn't hang unnecessarily.
	QProcess ffmpeg;
	ffmpeg.start(s, QStringList());
	if (!ffmpeg.waitForStarted(2000) || !ffmpeg.waitForFinished(3000))
		QMessageBox::warning(this, tr("Warning"), tr("Couldn't execute ffmpeg at given location. Thumbnailing will not work."));
}

void PreferencesMedia::on_ffmpegFile_clicked()
{
	QFileInfo fi(system_default_filename());
	QString ffmpegFileName = QFileDialog::getOpenFileName(this, tr("Select ffmpeg executable"));

	if (!ffmpegFileName.isEmpty()) {
		ui->ffmpegExecutable->setText(ffmpegFileName);
		checkFfmpegExecutable();
	}
}

void PreferencesMedia::on_ffmpegExecutable_editingFinished()
{
	checkFfmpegExecutable();
}

void PreferencesMedia::on_extractVideoThumbnails_toggled(bool toggled)
{
	ui->videoThumbnailPosition->setEnabled(toggled);
	ui->ffmpegExecutable->setEnabled(toggled);
	ui->ffmpegFile->setEnabled(toggled);
}

void PreferencesMedia::refreshSettings()
{
	ui->videoThumbnailPosition->setEnabled(qPrefMedia::extract_video_thumbnails());
	ui->ffmpegExecutable->setEnabled(qPrefMedia::extract_video_thumbnails());
	ui->ffmpegFile->setEnabled(qPrefMedia::extract_video_thumbnails());

	ui->extractVideoThumbnails->setChecked(qPrefMedia::extract_video_thumbnails());
	ui->videoThumbnailPosition->setValue(qPrefMedia::extract_video_thumbnails_position());
	ui->ffmpegExecutable->setText(qPrefMedia::ffmpeg_executable());

	ui->auto_recalculate_thumbnails->setChecked(prefs.auto_recalculate_thumbnails);
}

void PreferencesMedia::syncSettings()
{
	auto media = qPrefMedia::instance();
	media->set_extract_video_thumbnails(ui->extractVideoThumbnails->isChecked());
	media->set_extract_video_thumbnails_position(ui->videoThumbnailPosition->value());
	media->set_ffmpeg_executable(ui->ffmpegExecutable->text());
	qPrefMedia::set_auto_recalculate_thumbnails(ui->auto_recalculate_thumbnails->isChecked());
}
