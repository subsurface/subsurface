// SPDX-License-Identifier: GPL-2.0
#include "findmovedimagesdialog.h"
#include "core/picture.h"
#include "core/qthelper.h"
#include "desktop-widgets/divelistview.h"	// TODO: used for lastUsedImageDir()
#include "qt-models/divepicturemodel.h"

#include <QFileDialog>
#include <QtConcurrent>

FindMovedImagesDialog::FindMovedImagesDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	fontMetrics.reset(new QFontMetrics(ui.scanning->font()));
	connect(&watcher, &QFutureWatcher<QVector<Match>>::finished, this, &FindMovedImagesDialog::searchDone);
	connect(ui.buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &FindMovedImagesDialog::apply);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

// Compare two full paths and return the number of matching levels, starting from the filename.
// String comparison is case-insensitive.
static int matchPath(const QString &path1, const QString &path2)
{
	QFileInfo f1(path1);
	QFileInfo f2(path2);

	int score = 0;
	for (;;) {
		QString fn1 = f1.fileName();
		QString fn2 = f2.fileName();
		if (fn1.isEmpty() || fn2.isEmpty())
			break;
		if (fn1 == ".") {
			f1 = QFileInfo(f1.path());
			continue;
		}
		if (fn2 == ".") {
			f2 = QFileInfo(f2.path());
			continue;
		}
		if (QString::compare(fn1, fn2, Qt::CaseInsensitive) != 0)
			break;
		f1 = QFileInfo(f1.path());
		f2 = QFileInfo(f2.path());
		++score;
	}
	return score;
}

FindMovedImagesDialog::ImagePath::ImagePath(const QString &path) : fullPath(path),
	filenameUpperCase(QFileInfo(path).fileName().toUpper())
{
}

bool FindMovedImagesDialog::ImagePath::operator<(const ImagePath &path2) const
{
	return filenameUpperCase < path2.filenameUpperCase;
}

void FindMovedImagesDialog::learnImage(const QString &filename, QMap<QString, ImageMatch> &matches, const QVector<ImagePath> &imagePaths)
{
	QStringList newMatches;
	int bestScore = 1;
	// Find matching file paths by a binary search of the file name
	ImagePath path(filename);
	for (auto it = std::lower_bound(imagePaths.begin(), imagePaths.end(), path);
	     it != imagePaths.end() && it->filenameUpperCase == path.filenameUpperCase;
	     ++it) {
		int score = matchPath(filename, it->fullPath);
		if (score < bestScore)
			continue;
		if (score > bestScore)
			newMatches.clear();
		newMatches.append(it->fullPath);
		bestScore = score;
	}

	// Add the new original filenames to the list of matches, if the score is higher than previously
	for (const QString &originalFilename: newMatches) {
		auto it = matches.find(originalFilename);
		if (it == matches.end())
			matches.insert(originalFilename, { filename, bestScore });
		else if (it->score < bestScore)
			*it = { filename, bestScore };
	}
}

// We use a stack to recurse into directories. Each level of the stack is made up of
// a list of subdirectories to process. For each directory we keep track of the progress
// that is done when processing this directory. In principle the from value is redundant
// (it could be extracted from previous stack entries, but it makes code simpler.
struct Dir {
	QString path;
	double progressFrom, progressTo;
};

QVector<FindMovedImagesDialog::Match> FindMovedImagesDialog::learnImages(const QString &rootdir, int maxRecursions,
									 QVector<QString> imagePathsIn)
{
	QMap<QString, ImageMatch> matches;

	// For divelogs with thousands of images, we don't want to compare the path of every image.
	// Therefore, keep an array of image paths sorted by the filename in upper case.
	// Thus, we can access all paths ending in the same filename by a binary search. We suppose that
	// there aren't many pictures with the same filename but different paths.
	QVector<ImagePath> imagePaths;
	imagePaths.reserve(imagePathsIn.size());
	for (const QString &path: imagePathsIn)
		imagePaths.append(ImagePath(path));	// No emplace() in QVector? Sheesh.
	std::sort(imagePaths.begin(), imagePaths.end());

	// Free memory of original path vector - we don't need it any more
	imagePathsIn.clear();

	QVector<QVector<Dir>> stack; // Use a stack to recurse into directories
	stack.reserve(maxRecursions + 1);
	stack.append({ { rootdir, 0.0, 1.0 } });
	while (!stack.isEmpty()) {
		if (stack.last().isEmpty()) {
			stack.removeLast();
			continue;
		}
		Dir entry = stack.last().takeLast();
		QDir dir(entry.path);

		// Since we're running in a different thread, use invokeMethod to set progress.
		QMetaObject::invokeMethod(this, "setProgress", Q_ARG(double, entry.progressFrom), Q_ARG(QString, dir.absolutePath()));

		for (const QString &file: dir.entryList(QDir::Files)) {
			if (stopScanning != 0)
				goto out;
			learnImage(dir.absoluteFilePath(file), matches, imagePaths);
		}
		if (stack.size() <= maxRecursions) {
			stack.append(QVector<Dir>());
			QVector<Dir> &newItem = stack.last();
			for (const QString &dirname: dir.entryList(QDir::NoDotAndDotDot | QDir::Dirs))
				stack.last().append({ dir.filePath(dirname), 0.0, 0.0 });
			int num = newItem.size();
			double diff = entry.progressTo - entry.progressFrom;
			// We pop from back therefore we fill the progress in reverse
			for (int i = 0; i < num; ++i) {
				newItem[num - i - 1].progressFrom = (i / (double)num) * diff + entry.progressFrom;
				newItem[num - i - 1].progressTo = ((i + 1) / (double)num) * diff + entry.progressFrom;
			}
		}
	}
out:
	QMetaObject::invokeMethod(this, "setProgress", Q_ARG(double, 1.0), Q_ARG(QString, QString()));
	QVector<FindMovedImagesDialog::Match> ret;
	for (auto it = matches.begin(); it != matches.end(); ++it)
		ret.append({ it.key(), it->localFilename, it->score });
	return ret;
}

void FindMovedImagesDialog::setProgress(double progress, QString path)
{
	ui.progress->setValue((int)(progress * 100.0));

	// Elide text to avoid rescaling of the window if path is too long.
	// Note that we subtract an arbitrary 10 pixels from the width, because otherwise the label slowly grows.
	QString elidedPath = fontMetrics->elidedText(path, Qt::ElideMiddle, ui.scanning->width() - 10);
	ui.scanning->setText(elidedPath);
}

void FindMovedImagesDialog::on_scanButton_clicked()
{
	if (watcher.isRunning()) {
		stopScanning = 1;
		return;
	}

	// TODO: is lastUsedImageDir really well-placed in DiveListView?
	QString dirName = QFileDialog::getExistingDirectory(this,
							    tr("Traverse media directories"),
							    DiveListView::lastUsedImageDir(),
							    QFileDialog::ShowDirsOnly);
	if (dirName.isEmpty())
		return;
	DiveListView::updateLastUsedImageDir(dirName);
	ui.scanButton->setText(tr("Stop scanning"));
	ui.buttonBox->setEnabled(false);
	ui.imagesText->clear();
	// We have to collect the names of the image filenames in the main thread
	bool onlySelected = ui.onlySelectedDives->isChecked();
	QVector<QString> imagePaths;
	int i;
	struct dive *dive;
	for_each_dive (i, dive)
		if (!onlySelected || dive->selected)
			FOR_EACH_PICTURE(dive)
				imagePaths.append(QString(picture->filename));
	stopScanning = 0;
	QFuture<QVector<Match>> future = QtConcurrent::run(
			// Note that we capture everything but "this" by copy to avoid dangling references.
			[this, dirName, imagePaths]()
			{ return learnImages(dirName, 20, std::move(imagePaths)); }
	);
	watcher.setFuture(future);
}

static QString formatPath(const QString &path, int numBold)
{
	QString res;
	QVector<QString> boldPaths;
	boldPaths.reserve(numBold);
	QFileInfo info(path);
	for (int i = 0; i < numBold; ++i) {
		QString fn = info.fileName();
		if (fn.isEmpty())
			break;
		boldPaths.append(fn);
		info = QFileInfo(info.path());
	}
	QString nonBoldPath = info.filePath();
	QString separator = QDir::separator();
	if (!nonBoldPath.isEmpty()) {
		res += nonBoldPath.toHtmlEscaped();
		if (!boldPaths.isEmpty() && nonBoldPath[nonBoldPath.size() - 1] != QDir::separator())
			res += separator;
	}

	if (boldPaths.size() > 0) {
		res += "<b>";
		for (int i = boldPaths.size() - 1; i >= 0; --i) {
			res += boldPaths[i].toHtmlEscaped();
			if (i > 0)
				res += separator;
		}
		res += "</b>";
	}
	return res;
}

static bool sameFile(const QString &f1, const QString &f2)
{
	return QFileInfo(f1) == QFileInfo(f2);
}

void FindMovedImagesDialog::searchDone()
{
	ui.scanButton->setText(tr("Select folder and scan"));
	ui.buttonBox->setEnabled(true);
	ui.scanning->clear();

	matches = watcher.result();
	ui.imagesText->clear();

	QString text;
	int numChanged = 0;
	if (stopScanning != 0) {
		text += "<b>" + tr("Scanning cancelled - results may be incomplete") + "</b><br/>";
		stopScanning = 0;
	}
	if (matches.isEmpty()) {
		text += "<i>" + tr("No matching media files found") + "</i>";
	} else {
		QString matchesText;
		for (const Match &match: matches) {
			if (!sameFile(match.localFilename, localFilePath(match.originalFilename))) {
				++numChanged;
				matchesText += formatPath(match.originalFilename, match.matchingPathItems) + " → " +
					       formatPath(match.localFilename, match.matchingPathItems) + "<br/>";
			}
		}
		int numUnchanged = matches.size() - numChanged;
		if (numUnchanged > 0)
			text += tr("Found <b>%1</b> media files at their current place.").arg(numUnchanged) + "<br/>";
		if (numChanged > 0) {
			text += tr("Found <b>%1</b> media files at new locations:").arg(numChanged) + "<br/>";
			text += matchesText;
		}
	}
	ui.imagesText->setHtml(text);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(numChanged > 0);
}

void FindMovedImagesDialog::apply()
{
	for (const Match &match: matches)
		learnPictureFilename(match.originalFilename, match.localFilename);
	write_hashes();
	DivePictureModel::instance()->updateDivePictures();

	ui.imagesText->clear();
	matches.clear();
	hide();
	close();
}

void FindMovedImagesDialog::on_buttonBox_rejected()
{
	ui.imagesText->clear();
	matches.clear();
}
