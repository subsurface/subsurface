// SPDX-License-Identifier: GPL-2.0
#include "dive.h"
#include "metrics.h"
#include "divelist.h"
#include "qthelper.h"
#include "imagedownloader.h"
#include "videoframeextractor.h"
#include "qt-models/divepicturemodel.h"
#include "metadata.h"
#include <unistd.h>
#include <QString>
#include <QImageReader>
#include <QSvgRenderer>
#include <QDataStream>
#include <QPainter>

#include <QtConcurrent>

// Note: this is a global instead of a function-local variable on purpose.
// We don't want this to be generated in a different thread context if
// ImageDownloader::instance() is called from a worker thread.
static ImageDownloader imageDownloader;
ImageDownloader *ImageDownloader::instance()
{
	return &imageDownloader;
}

ImageDownloader::ImageDownloader()
{
	connect(&manager, &QNetworkAccessManager::finished, this, &ImageDownloader::saveImage);
}

void ImageDownloader::load(QUrl url, QString filename)
{
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::User, filename);
	manager.get(request);
}

void ImageDownloader::saveImage(QNetworkReply *reply)
{
	QString filename = reply->request().attribute(QNetworkRequest::User).toString();

	if (reply->error() != QNetworkReply::NoError) {
		emit failed(std::move(filename));
	} else {
		QByteArray imageData = reply->readAll();
		if (imageData.isEmpty()) {
			emit failed(std::move(filename));
		} else {
			QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first();
			QDir dir(path);
			if (!dir.exists())
				dir.mkpath(path);
			QCryptographicHash hash(QCryptographicHash::Sha1);
			hash.addData(filename.toUtf8());
			QFile imageFile(path.append("/").append(hash.result().toHex()));
			if (imageFile.open(QIODevice::WriteOnly)) {
				qDebug() << "Write image to" << imageFile.fileName();
				QDataStream stream(&imageFile);
				stream.writeRawData(imageData.data(), imageData.length());
				imageFile.waitForBytesWritten(-1);
				imageFile.close();
				learnPictureFilename(filename, imageFile.fileName());
			}
			emit loaded(std::move(filename));
		}
	}

	reply->deleteLater();
}

static bool hasVideoFileExtension(const QString &filename)
{
	for (const QString &ext: videoExtensionsList)
		if (filename.endsWith(ext, Qt::CaseInsensitive))
			return true;
	return false;
}

// Fetch a picture from the given filename and determine its type (picture of video).
// If this is a non-remote file, fetch it from disk. Remote files are fetched from the
// net in a background thread. In such a case, the output-type is set to MEDIATYPE_STILL_LOADING.
// If the input-flag "tryDownload" is set to false, no download attempt is made. This is to
// prevent infinite loops, where failed image downloads would be repeated ad infinitum.
// Returns: fetched image, type
Thumbnailer::Thumbnail Thumbnailer::fetchImage(const QString &urlfilename, const QString &originalFilename, bool tryDownload)
{
	QUrl url = QUrl::fromUserInput(urlfilename);
	if (url.isLocalFile()) {
		// We try to determine the type first by peeking into the file.
		QString filename = url.toLocalFile();
		metadata md;
		mediatype_t type = get_metadata(qPrintable(filename), &md);

		// For io error or video, return early with the appropriate dummy-icon.
		if (type == MEDIATYPE_IO_ERROR)
			return { failImage, MEDIATYPE_IO_ERROR, zero_duration };
		else if (type == MEDIATYPE_VIDEO)
			return fetchVideoThumbnail(filename, originalFilename, md.duration);

		// Try if Qt can parse this image. If it does, use this as a thumbnail.
		QImage thumb(filename);
		if (!thumb.isNull()) {
			int size = maxThumbnailSize();
			thumb = thumb.scaled(size, size, Qt::KeepAspectRatio);
			return addPictureThumbnailToCache(originalFilename, thumb);
		}

		// Neither our code, nor Qt could determine the type of this object from looking at the data.
		// Try to check for a video-file extension. Since we couldn't parse the video file,
		// we pass 0 as the duration.
		if (hasVideoFileExtension(filename))
			return fetchVideoThumbnail(filename, originalFilename, zero_duration);

		// Give up: we simply couldn't determine what this thing is.
		// But since we managed to read this file, mark this file in the cache as unknown.
		return addUnknownThumbnailToCache(originalFilename);
	} else if (tryDownload) {
		// This has to be done in UI main thread, because QNetworkManager refuses
		// to treat requests from other threads. invokeMethod() is Qt's way of calling a
		// function in a different thread, namely the thread the called object is associated to.
		QMetaObject::invokeMethod(ImageDownloader::instance(), "load", Qt::AutoConnection, Q_ARG(QUrl, url), Q_ARG(QString, originalFilename));
		return { QImage(), MEDIATYPE_STILL_LOADING, zero_duration };
	}
	return { QImage(), MEDIATYPE_IO_ERROR, zero_duration };
}

// Fetch a picture based on its original filename. If there is a translated filename (obtained either
// by the find-moved-picture functionality or the filename of the local cache of a remote picture),
// try that first. If fetching from the translated filename fails, this could mean that the image
// was downloaded previously, but for some reason the cached picture was lost. Therefore, in such a
// case, try the canonical filename. If that likewise fails, give up. For input and output parameters
// see fetchImage() above.
Thumbnailer::Thumbnail Thumbnailer::getHashedImage(const QString &filename, bool tryDownload)
{
	QString localFilename = localFilePath(filename);

	// If there is a translated filename, try that first.
	// Note that we set the default type to io-error, so that if we didn't try
	// the local filename first, we will load the file from the canonical filename.
	Thumbnail thumbnail { QImage(), MEDIATYPE_IO_ERROR, zero_duration };
	if (localFilename != filename)
		thumbnail = fetchImage(localFilename, filename, tryDownload);

	// If fetching from the local filename failed (or we didn't even try),
	// use the canonical filename. This might for example happen if we downloaded
	// a file, but for some reason lost the cached file.
	if (thumbnail.type == MEDIATYPE_IO_ERROR)
		thumbnail = fetchImage(filename, filename, tryDownload);

	if (thumbnail.type == MEDIATYPE_IO_ERROR)
		qInfo() << "Error loading image" << filename << "[local:" << localFilename << "]";
	return thumbnail;
}

Thumbnailer::Thumbnailer() : failImage(QPixmap(":filter-close").scaled(maxThumbnailSize(), maxThumbnailSize(), Qt::KeepAspectRatio).toImage()), // TODO: Don't misuse filter close icon
			     dummyImage(QPixmap(":camera-icon").scaled(maxThumbnailSize(), maxThumbnailSize(), Qt::KeepAspectRatio).toImage()),
			     videoImage(QPixmap(":video-icon").scaled(maxThumbnailSize(), maxThumbnailSize(), Qt::KeepAspectRatio).toImage()),
			     unknownImage(QPixmap(":unknown-icon").scaled(maxThumbnailSize(), maxThumbnailSize(), Qt::KeepAspectRatio).toImage())
{
	// We have to do this little song and dance because QSvgRenderer produces artifacts when used with Qt::KeepAspectRatio
	QSvgRenderer videoOverlayRenderer{QString(":video-overlay")};
	QSize svgSize = videoOverlayRenderer.defaultSize();
	videoOverlayImage = QImage(maxThumbnailSize(), maxThumbnailSize() * svgSize.height() / svgSize.width(), QImage::Format_ARGB32);
	videoOverlayImage.fill(Qt::transparent);
	QPainter painter(&videoOverlayImage);
	videoOverlayRenderer.render(&painter);
	// Currently, we only process one image at a time. Stefan Fuchs reported problems when
	// calculating multiple thumbnails at once and this hopefully helps.
	pool.setMaxThreadCount(1);
	connect(ImageDownloader::instance(), &ImageDownloader::loaded, this, &Thumbnailer::imageDownloaded);
	connect(ImageDownloader::instance(), &ImageDownloader::failed, this, &Thumbnailer::imageDownloadFailed);
	connect(VideoFrameExtractor::instance(), &VideoFrameExtractor::extracted, this, &Thumbnailer::frameExtracted);
	connect(VideoFrameExtractor::instance(), &VideoFrameExtractor::failed, this, &Thumbnailer::frameExtractionFailed);
	connect(VideoFrameExtractor::instance(), &VideoFrameExtractor::failed, this, &Thumbnailer::frameExtractionInvalid);
}

Thumbnailer *Thumbnailer::instance()
{
	static Thumbnailer self;
	return &self;
}

Thumbnailer::Thumbnail Thumbnailer::getPictureThumbnailFromStream(QDataStream &stream)
{
	QImage res;
	stream >> res;
	return { std::move(res), MEDIATYPE_PICTURE, zero_duration };
}

void Thumbnailer::markVideoThumbnail(QImage &img)
{
	QSize size = img.size();
	QImage marker = videoOverlayImage.scaledToWidth(size.width());
	marker = marker.copy(0, (marker.size().height() - size.height()) / 2, size.width(), size.height());
	QPainter painter(&img);
	painter.drawImage(0, 0, marker);
}

Q_DECLARE_METATYPE(duration_t)
Thumbnailer::Thumbnail Thumbnailer::getVideoThumbnailFromStream(QDataStream &stream, const QString &filename)
{
	quint32 duration, numPics;
	stream >> duration >> numPics;

	// If reading did not succeed, schedule for recalculation - this thumbnail might
	// have been written by an older version, which couldn't extract the duration.
	// Likewise test the duration and number of pictures for sanity (no videos longer than 10 h,
	// no more than 10000 pictures).
	if (stream.status() != QDataStream::Ok || duration > 36000 || numPics > 10000)
		return { QImage(), MEDIATYPE_VIDEO, zero_duration };

	// If the file didn't contain an image, but user turned on thumbnail extraction, schedule thumbnail
	// for extraction. TODO: save failure to extract thumbnails to disk so that thumbnailing
	// is not repeated ad-nauseum for broken images.
	if (numPics == 0 && prefs.extract_video_thumbnails) {
		QMetaObject::invokeMethod(VideoFrameExtractor::instance(), "extract", Qt::AutoConnection,
					  Q_ARG(QString, filename), Q_ARG(QString, filename), Q_ARG(duration_t, duration_t{(int32_t)duration}));
	}

	// Currently, we support only one picture
	QImage res;
	if (numPics > 0) {
		quint32 offset;
		stream >> offset >> res;
	}

	if (res.isNull())
		res = videoImage; // No picture -> show dummy-icon
	else
		markVideoThumbnail(res); // We got an image -> place our video marker on top of it
	return { res, MEDIATYPE_VIDEO, { (int32_t)duration } };
}

// Fetch a thumbnail from cache.
// If Thumbnail::QImage is null, the thumbnail is scheduled for recreation.
Thumbnailer::Thumbnail Thumbnailer::getThumbnailFromCache(const QString &picture_filename)
{
	QString filename = thumbnailFileName(picture_filename);
	if (filename.isEmpty())
		return { QImage(), MEDIATYPE_UNKNOWN, zero_duration };
	QFile file(filename);

	if (prefs.auto_recalculate_thumbnails) {
		// Check if thumbnails is older than the (local) image file
		QString filenameLocal = localFilePath(qPrintable(picture_filename));
		QFileInfo pictureInfo(filenameLocal);
		QFileInfo thumbnailInfo(file);
		if (pictureInfo.exists() && thumbnailInfo.exists()) {
			QDateTime pictureTime = pictureInfo.lastModified();
			QDateTime thumbnailTime = thumbnailInfo.lastModified();
			if (pictureTime.isValid() && thumbnailTime.isValid() && thumbnailTime < pictureTime) {
				// Both files exist, have valid timestamps and thumbnail was calculated before picture.
				// Return an empty thumbnail to signal recalculation of the thumbnail
				return { QImage(), MEDIATYPE_UNKNOWN, zero_duration };
			}
		}
	}

	if (!file.open(QIODevice::ReadOnly))
		return { QImage(), MEDIATYPE_UNKNOWN, zero_duration };
	QDataStream stream(&file);

	// Each thumbnail file is composed of a media-type and an image file.
	quint32 type;
	QImage res;
	stream >> type;

	switch (type) {
	case MEDIATYPE_PICTURE:	return getPictureThumbnailFromStream(stream);
	case MEDIATYPE_VIDEO:	return getVideoThumbnailFromStream(stream, picture_filename);
	case MEDIATYPE_UNKNOWN:	return { unknownImage, MEDIATYPE_UNKNOWN, zero_duration };
	default:		return { QImage(), MEDIATYPE_UNKNOWN, zero_duration };
	}
}

Thumbnailer::Thumbnail Thumbnailer::addVideoThumbnailToCache(const QString &picture_filename, duration_t duration,
							     const QImage &image, duration_t position)
{
	// The format of video thumbnails:
	//	uint32	MEDIATYPE_VIDEO
	//	uint32	duration of video in seconds
	//	uint32	number of pictures (0 = we didn't manage to extract a picture)
	//	for each picture:
	//		uint32	offset in msec from begining of video
	//		QImage	frame
	QString filename = thumbnailFileName(picture_filename);
	QSaveFile file(filename);
	if (file.open(QIODevice::WriteOnly)) {
		QDataStream stream(&file);

		stream << (quint32)MEDIATYPE_VIDEO;
		stream << (quint32)duration.seconds;

		if (image.isNull()) {
			// No image provided
			stream << (quint32)0;
		} else {
			// Currently, we support at most one image
			stream << (quint32)1;
			stream << (quint32)position.seconds;
			stream << image;
		}

		file.commit();
	}
	return { videoImage, MEDIATYPE_VIDEO, duration };
}

Thumbnailer::Thumbnail Thumbnailer::fetchVideoThumbnail(const QString &filename, const QString &originalFilename, duration_t duration)
{
	if (prefs.extract_video_thumbnails) {
		// Video-thumbnailing is enabled. Fetch thumbnail in background thread and in the meanwhile
		// return a dummy image.
		QMetaObject::invokeMethod(VideoFrameExtractor::instance(), "extract", Qt::AutoConnection,
					  Q_ARG(QString, originalFilename), Q_ARG(QString, filename), Q_ARG(duration_t, duration));
		return { videoImage, MEDIATYPE_VIDEO, duration };
	} else {
		// Video-thumbnailing is disabled. Write a thumbnail without picture.
		return addVideoThumbnailToCache(originalFilename, duration, QImage(), zero_duration);
	}
}

Thumbnailer::Thumbnail Thumbnailer::addPictureThumbnailToCache(const QString &picture_filename, const QImage &thumbnail)
{
	// The format of a picture-thumbnail is very simple:
	// 	uint32	MEDIATYPE_PICTURE
	// 	QImage	thumbnail
	QString filename = thumbnailFileName(picture_filename);
	QSaveFile file(filename);
	if (file.open(QIODevice::WriteOnly)) {
		QDataStream stream(&file);

		stream << (quint32)MEDIATYPE_PICTURE;
		stream << thumbnail;
		file.commit();
	}
	return { thumbnail, MEDIATYPE_PICTURE, zero_duration };
}

Thumbnailer::Thumbnail Thumbnailer::addUnknownThumbnailToCache(const QString &picture_filename)
{
	QString filename = thumbnailFileName(picture_filename);
	QSaveFile file(filename);
	if (file.open(QIODevice::WriteOnly)) {
		QDataStream stream(&file);
		stream << (quint32)MEDIATYPE_UNKNOWN;
	}
	return { unknownImage, MEDIATYPE_UNKNOWN, zero_duration };
}

void Thumbnailer::frameExtracted(QString filename, QImage thumbnail, duration_t duration, duration_t offset)
{
	if (thumbnail.isNull()) {
		frameExtractionFailed(std::move(filename), duration);
		return;
	} else {
		int size = maxThumbnailSize();
		thumbnail = thumbnail.scaled(size, size, Qt::KeepAspectRatio);
		markVideoThumbnail(thumbnail);
		addVideoThumbnailToCache(filename, duration, thumbnail, offset);
		QMutexLocker l(&lock);
		workingOn.remove(filename);
		emit thumbnailChanged(filename, std::move(thumbnail), duration);
	}
}

// If frame extraction failed, don't show an error image, because we don't want
// to penalize users that haven't installed ffmpe. Simply remove this item from
// the work-queue.
void Thumbnailer::frameExtractionFailed(QString filename, duration_t duration)
{
	// Frame extraction failed, but this was due to ffmpeg not starting
	// add to the thumbnail cache as a video image with unknown thumbnail.
	addVideoThumbnailToCache(filename, duration, QImage(), zero_duration);
	QMutexLocker l(&lock);
	workingOn.remove(filename);
}

void Thumbnailer::frameExtractionInvalid(QString filename, duration_t)
{
	// Frame extraction failed because ffmpeg could not parse the file.
	// For now, let's mark this as an unknown file. The user may want
	// to recalculate thumbnails with an updated ffmpeg binary..?
	addUnknownThumbnailToCache(filename);
	QMutexLocker l(&lock);
	workingOn.remove(filename);
}

void Thumbnailer::recalculate(QString filename)
{
	Thumbnail thumbnail = getHashedImage(filename, true);

	// If we couldn't load the image from disk -> leave old thumbnail.
	// The case "load from web" is a bit inconsistent: it will call into processItem() later
	// and therefore a "broken" image symbol may be shown.
	if (thumbnail.type == MEDIATYPE_STILL_LOADING || thumbnail.type == MEDIATYPE_IO_ERROR)
		return;

	QMutexLocker l(&lock);
	emit thumbnailChanged(filename, thumbnail.img, thumbnail.duration);
	workingOn.remove(filename);
}

void Thumbnailer::processItem(QString filename, bool tryDownload)
{
	Thumbnail thumbnail = getThumbnailFromCache(filename);

	if (thumbnail.img.isNull()) {
		thumbnail = getHashedImage(filename, tryDownload);
		if (thumbnail.type == MEDIATYPE_STILL_LOADING)
			return;

		if (thumbnail.img.isNull()) {
			thumbnail.img = failImage;
		} else {
			int size = maxThumbnailSize();
			thumbnail.img = thumbnail.img.scaled(size, size, Qt::KeepAspectRatio);
		}
	}

	QMutexLocker l(&lock);
	emit thumbnailChanged(filename, thumbnail.img, thumbnail.duration);
	workingOn.remove(filename);
}

void Thumbnailer::imageDownloaded(QString filename)
{
	// Image was downloaded -> try thumbnailing again.
	QMutexLocker l(&lock);
	workingOn[filename] = QtConcurrent::run(&pool, [this, filename]() { processItem(filename, false); });
}

void Thumbnailer::imageDownloadFailed(QString filename)
{
	emit thumbnailChanged(filename, failImage, zero_duration);
	QMutexLocker l(&lock);
	workingOn.remove(filename);
}

QImage Thumbnailer::fetchThumbnail(const QString &filename, bool synchronous)
{
	if (synchronous) {
		// In synchronous mode, first try the thumbnail cache.
		Thumbnail thumbnail = getThumbnailFromCache(filename);
		if (!thumbnail.img.isNull())
			return thumbnail.img;

		// If that didn't work, try to thumbnail the image.
		thumbnail = getHashedImage(filename, false);
		if (thumbnail.type == MEDIATYPE_STILL_LOADING || thumbnail.img.isNull())
			return failImage; // No support for delayed thumbnails (web).

		int size = maxThumbnailSize();
		return thumbnail.img.scaled(size, size, Qt::KeepAspectRatio);
	}

	QMutexLocker l(&lock);

	// We are not currently fetching this thumbnail - add it to the list.
	if (!workingOn.contains(filename)) {
		workingOn.insert(filename,
				 QtConcurrent::run(&pool, [this, filename]() { processItem(filename, true); }));
	}
	return dummyImage;
}

void Thumbnailer::calculateThumbnails(const QVector<QString> &filenames)
{
	QMutexLocker l(&lock);
	for (const QString &filename: filenames) {
		if (!workingOn.contains(filename)) {
			workingOn.insert(filename,
					 QtConcurrent::run(&pool, [this, filename]() { recalculate(filename); }));
		}
	}
}

void Thumbnailer::clearWorkQueue()
{
	// We also want to clear the working-queue of the video-frame-extractor so that
	// we don't get thumbnails that we don't care about.
	VideoFrameExtractor::instance()->clearWorkQueue();

	QMutexLocker l(&lock);
	for (auto it = workingOn.begin(); it != workingOn.end(); ++it)
		it->cancel();
	workingOn.clear();
}

static const int maxZoom = 3;	// Maximum zoom: thrice of standard size

int Thumbnailer::defaultThumbnailSize()
{
	return defaultIconMetrics().sz_pic;
}

int Thumbnailer::maxThumbnailSize()
{
	return defaultThumbnailSize() * maxZoom;
}

int Thumbnailer::thumbnailSize(double zoomLevel)
{
	// Calculate size of thumbnails. The standard size is defaultIconMetrics().sz_pic.
	// We use exponential scaling so that the central point is the standard
	// size and the minimum and maximum extreme points are a third respectively
	// three times the standard size.
	// Naturally, these three zoom levels are then represented by
	// -1.0 (minimum), 0 (standard) and 1.0 (maximum). The actual size is
	// calculated as standard_size*3.0^zoomLevel.
	return static_cast<int>(round(defaultThumbnailSize() * pow(maxZoom, zoomLevel)));
}
