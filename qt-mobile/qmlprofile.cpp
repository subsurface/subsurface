#include "qmlprofile.h"
#include "profile-widget/profilewidget2.h"
#include "subsurface-core/dive.h"
#include <QTransform>

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent)
{
	m_profileWidget = new ProfileWidget2(0);
	m_profileWidget->setProfileState();
	m_profileWidget->setPrintMode(true);
	m_profileWidget->setFontPrintScale(0.8);
	//m_profileWidget->setGeometry(this->geometry());
}

QMLProfile::~QMLProfile()
{
	m_profileWidget->deleteLater();
}

void QMLProfile::paint(QPainter *painter)
{
	m_profileWidget->render(painter);
}

QString QMLProfile::diveId() const
{
	return m_diveId;
}

void QMLProfile::setDiveId(const QString &diveId)
{
	static bool firstRun = true;
	static QTransform profileTransform;
	m_diveId = diveId;
	struct dive *d = get_dive_by_uniq_id(m_diveId.toInt());
	if (m_diveId.toInt() < 1)
		return;
	if (!d)
		return;

	qDebug() << "setDiveId called with pos/size" << x() << y() << width() << height();
	// set the profile widget's geometry and scale the viewport so
	// the scene fills it, then plot the dive on that widget
	if (firstRun) {
		firstRun = false;
		m_profileWidget->setGeometry(QRect(x(), y(), width(), height()));
		profileTransform.scale(width() / 100, height() / 100);
	}
	m_profileWidget->setTransform(profileTransform);
	qDebug() << "effective transformation:" <<
		    m_profileWidget->transform().m11() <<
		    m_profileWidget->transform().m12() <<
		    m_profileWidget->transform().m13() <<
		    m_profileWidget->transform().m21() <<
		    m_profileWidget->transform().m22() <<
		    m_profileWidget->transform().m23() <<
		    m_profileWidget->transform().m31() <<
		    m_profileWidget->transform().m32() <<
		    m_profileWidget->transform().m33();

	m_profileWidget->plotDive(d);
}
