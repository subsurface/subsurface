#include "statscolors.h"
#include "statstranslations.h"
#include "core/globals.h"
#include <array>

// Colors created using the Chroma.js Color Palette Helper
// https://vis4.net/palettes/#/50|d|00108c,3ed8ff,ffffe0|ffffe0,ff005e,743535|1|1
static const QColor binColors[] = {
	QRgb(0x00108c), QRgb(0x0f1c92), QRgb(0x1a2798), QRgb(0x23319d), QRgb(0x2a3ba3),
	QRgb(0x3144a8), QRgb(0x374eae), QRgb(0x3e58b3), QRgb(0x4461b8), QRgb(0x4b6bbd),
	QRgb(0x5274c2), QRgb(0x587ec7), QRgb(0x6088cc), QRgb(0x6791d0), QRgb(0x6f9bd4),
	QRgb(0x78a5d8), QRgb(0x81aedb), QRgb(0x8ab8df), QRgb(0x95c2e2), QRgb(0xa0cbe4),
	QRgb(0xacd5e6), QRgb(0xb9dee7), QRgb(0xc7e7e7), QRgb(0xd7efe7), QRgb(0xeaf8e4),
	QRgb(0xfff5d8), QRgb(0xffead0), QRgb(0xffe0c8), QRgb(0xffd5c0), QRgb(0xffcab8),
	QRgb(0xffbfb0), QRgb(0xffb4a8), QRgb(0xffa99f), QRgb(0xfc9e98), QRgb(0xf99490),
	QRgb(0xf48b89), QRgb(0xf08182), QRgb(0xea787b), QRgb(0xe46f74), QRgb(0xde666e),
	QRgb(0xd75e67), QRgb(0xcf5661), QRgb(0xc64f5b), QRgb(0xbd4855), QRgb(0xb3434f),
	QRgb(0xa83e49), QRgb(0x9d3a44), QRgb(0x90383f), QRgb(0x83363a), QRgb(0x743535)
};

StatsTheme::StatsTheme() :
	scatterItemTexture(nullptr),
	scatterItemSelectedTexture(nullptr),
	scatterItemHighlightedTexture(nullptr),
	selectedTexture(nullptr)
{
}

class StatsThemeLight : public StatsTheme {
public:
	StatsThemeLight()
	{
		backgroundColor = Qt::white;
		fillColor = QColor(0x44, 0x76, 0xaa);
		borderColor = QColor(0x66, 0xb2, 0xff);
		selectedColor = QColor(0xaa, 0x76, 0x44);
		selectedBorderColor = QColor(0xff, 0xb2, 0x66);
		highlightedColor = Qt::yellow;
		highlightedBorderColor = QColor(0xaa, 0xaa, 0x22);
		darkLabelColor = Qt::black;
		lightLabelColor = Qt::white;
		axisColor = Qt::black;
		gridColor = QColor(0xcc, 0xcc, 0xcc);
		informationBorderColor = Qt::black;
		informationColor = QColor(0xff, 0xff, 0x00, 192); // Note: fourth argument is opacity
		legendColor = QColor(0x00, 0x8e, 0xcc, 192); // Note: fourth argument is opacity
		legendBorderColor = Qt::black;
		quartileMarkerColor = Qt::red;
		regressionItemColor = Qt::red;
		meanMarkerColor = Qt::green;
		medianMarkerColor = Qt::red;
		selectionLassoColor = Qt::black;
		selectionOverlayColor = Qt::lightGray;

		// use a light version of the application font for axis labels, axis title and chart title
		axisLabelFont.setWeight(QFont::Light);
		axisTitleFont.setWeight(QFont::Light);
		titleFont.setWeight(QFont::Light);
	}
private:
	QString name() const
	{
		return StatsTranslations::tr("Light");
	}

	// Pick roughly equidistant colors out of the color set above
	// if we need more bins than we have colors (what chart is THAT?) simply loop
	QColor binColor(int bin, int numBins) const override
	{
		if (numBins == 1 || bin < 0 || bin >= numBins)
			return fillColor;
		if (numBins > (int)std::size(binColors))
			return binColors[bin % std::size(binColors)];

		// use integer math to spread out the indices
		int idx = bin * (std::size(binColors) - 1) / (numBins - 1);
		return binColors[idx];
	}

	QColor labelColor(int bin, size_t numBins) const override
	{
		return (binColor(bin, numBins).lightness() < 150) ? lightLabelColor : darkLabelColor;
	}
};

// we have a separate set of dark colors - but in reality te color palette the we created
// works well in either dark or light mode, so these are the same as for the light theme
static const QColor binColorsDark[] = {
	QRgb(0x00108c), QRgb(0x0f1c92), QRgb(0x1a2798), QRgb(0x23319d), QRgb(0x2a3ba3),
	QRgb(0x3144a8), QRgb(0x374eae), QRgb(0x3e58b3), QRgb(0x4461b8), QRgb(0x4b6bbd),
	QRgb(0x5274c2), QRgb(0x587ec7), QRgb(0x6088cc), QRgb(0x6791d0), QRgb(0x6f9bd4),
	QRgb(0x78a5d8), QRgb(0x81aedb), QRgb(0x8ab8df), QRgb(0x95c2e2), QRgb(0xa0cbe4),
	QRgb(0xacd5e6), QRgb(0xb9dee7), QRgb(0xc7e7e7), QRgb(0xd7efe7), QRgb(0xeaf8e4),
	QRgb(0xfff5d8), QRgb(0xffead0), QRgb(0xffe0c8), QRgb(0xffd5c0), QRgb(0xffcab8),
	QRgb(0xffbfb0), QRgb(0xffb4a8), QRgb(0xffa99f), QRgb(0xfc9e98), QRgb(0xf99490),
	QRgb(0xf48b89), QRgb(0xf08182), QRgb(0xea787b), QRgb(0xe46f74), QRgb(0xde666e),
	QRgb(0xd75e67), QRgb(0xcf5661), QRgb(0xc64f5b), QRgb(0xbd4855), QRgb(0xb3434f),
	QRgb(0xa83e49), QRgb(0x9d3a44), QRgb(0x90383f), QRgb(0x83363a), QRgb(0x743535)
};

class StatsThemeDark : public StatsTheme {
public:
	StatsThemeDark()
	{
		backgroundColor = Qt::black;
		fillColor = QColor(0x64, 0x96, 0xca);
		borderColor = QColor(0x66, 0xb2, 0xff);
		selectedColor = QColor(0x55, 0x89, 0xbb);
		selectedBorderColor = QColor(0x00, 0x4d, 0x99);
		highlightedColor = Qt::yellow;
		highlightedBorderColor = QColor(0x55, 0x55, 0xdd);
		darkLabelColor = Qt::white;
		lightLabelColor = Qt::black;
		axisColor = Qt::white;
		gridColor = QColor(0x33, 0x33, 0x33);
		informationBorderColor = Qt::white;
		informationColor = QColor(0x00, 0x00, 0xff, 192); // Note: fourth argument is opacity
		legendColor = QColor(0xff, 0x71, 0x33, 192); // Note: fourth argument is opacity
		legendBorderColor = Qt::white;
		quartileMarkerColor = Qt::cyan;
		regressionItemColor = Qt::cyan;
		meanMarkerColor = Qt::magenta;
		medianMarkerColor = Qt::cyan;
		selectionLassoColor = Qt::white;
		selectionOverlayColor = Qt::darkGray;

		// use a bold version of the application font for axis labels, axis title and chart title
		axisLabelFont.setWeight(QFont::Bold);
		axisTitleFont.setWeight(QFont::Bold);
		titleFont.setWeight(QFont::Bold);
	}
private:
	QString name() const
	{
		return StatsTranslations::tr("Dark");
	}

	// Pick roughly equidistant colors out of the color set above
	// if we need more bins than we have colors (what chart is THAT?) simply loop
	QColor binColor(int bin, int numBins) const override
	{
		if (numBins == 1 || bin < 0 || bin >= numBins)
			return fillColor;
		if (numBins > (int)std::size(binColorsDark))
			return binColorsDark[bin % std::size(binColorsDark)];

		// use integer math to spread out the indices
		int idx = bin * (std::size(binColorsDark) - 1) / (numBins - 1);
		return binColorsDark[idx];
	}

	QColor labelColor(int bin, size_t numBins) const override
	{
		return (binColor(bin, numBins).lightness() < 150) ? darkLabelColor : lightLabelColor;
	}
};

// Currently, we only support two themes: bright and dark.
// The themes are generated on first use. Thus, the constructors are run
// once the overall application is initialized. This ensures that the themes'
// constructors can access the settings, etc.
static std::array<const StatsTheme *, 2> statsThemes;
const StatsTheme &getStatsTheme(bool dark)
{
	if (!statsThemes[0]) {
		statsThemes[0] = make_global<StatsThemeLight>();
		statsThemes[1] = make_global<StatsThemeDark>();
	}
	return *statsThemes[dark ? 1 : 0];
}
