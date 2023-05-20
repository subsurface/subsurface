// SPDX-License-Identifier: GPL-2.0

#include "profilewidget.h"
#include "profile-widget/profileview.h"
#include "commands/command.h"
#include "core/color.h"
#include "core/selection.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefPartialPressureGas.h"
#include "core/subsurface-string.h"
#include "qt-models/diveplannermodel.h"

#include <QToolBar>
#include <QHBoxLayout>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QStackedWidget>
#include <QLabel>

// A resizing display of the Subsurface logo when no dive is shown
class EmptyView : public QLabel {
public:
	EmptyView(QWidget *parent = nullptr);
	~EmptyView();
private:
	QPixmap logo;
	void update();
	void resizeEvent(QResizeEvent *) override;
};

EmptyView::EmptyView(QWidget *parent) : QLabel(parent),
	logo(":poster-icon")
{
	QPalette pal;
	pal.setColor(QPalette::Window, getColor(::BACKGROUND));
	setAutoFillBackground(true);
	setPalette(pal);
	setMinimumSize(1,1);
	setAlignment(Qt::AlignHCenter);
	update();
}

EmptyView::~EmptyView()
{
}

void EmptyView::update()
{
	setPixmap(logo.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void EmptyView::resizeEvent(QResizeEvent *)
{
	update();
}

static const QUrl urlProfileView = QUrl(QStringLiteral("qrc:/qml/profileview.qml"));
ProfileWidget::ProfileWidget() : d(nullptr), dc(0), originalDive(nullptr), placingCommand(false)
{
	ui.setupUi(this);

	// what is a sane order for those icons? we should have the ones the user is
	// most likely to want towards the top so they are always visible
	// and the ones that someone likely sets and then never touches again towards the bottom
	toolbarActions = { ui.profInfobox, // show / hide the infobox
			   ui.profCalcCeiling, ui.profCalcAllTissues, // various ceilings
			   ui.profIncrement3m, ui.profDcCeiling,
			   ui.profPhe, ui.profPn2, ui.profPO2, // partial pressure graphs
			   ui.profRuler, ui.profScaled, // measuring and scaling
			   ui.profTogglePicture, ui.profTankbar,
			   ui.profMod, ui.profDeco, ui.profNdl_tts, // various values that a user is either interested in or not
			   ui.profEad, ui.profSAC,
			   ui.profHR, // very few dive computers support this
			   ui.profTissues }; // maybe less frequently used

	emptyView.reset(new EmptyView);

	viewWidget.reset(new QQuickWidget);
	viewWidget->setSource(urlProfileView);
	viewWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

	QToolBar *toolBar = new QToolBar(this);
	for (QAction *a: toolbarActions)
		toolBar->addAction(a);
	toolBar->setOrientation(Qt::Vertical);
	toolBar->setIconSize(QSize(24, 24));

	stack = new QStackedWidget(this);
	stack->addWidget(emptyView.get());
	stack->addWidget(viewWidget.get());

	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setSpacing(0);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	layout->setMargin(0);
#endif
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(toolBar);
	layout->addWidget(stack);
	setLayout(layout);

	// Toolbar Connections related to the Profile Update
	auto tec = qPrefTechnicalDetails::instance();
	connect(ui.profCalcAllTissues, &QAction::triggered, tec, &qPrefTechnicalDetails::set_calcalltissues);
	connect(ui.profCalcCeiling,    &QAction::triggered, tec, &qPrefTechnicalDetails::set_calcceiling);
	connect(ui.profDcCeiling,      &QAction::triggered, tec, &qPrefTechnicalDetails::set_dcceiling);
	connect(ui.profEad,            &QAction::triggered, tec, &qPrefTechnicalDetails::set_ead);
	connect(ui.profIncrement3m,    &QAction::triggered, tec, &qPrefTechnicalDetails::set_calcceiling3m);
	connect(ui.profMod,            &QAction::triggered, tec, &qPrefTechnicalDetails::set_mod);
	connect(ui.profNdl_tts,        &QAction::triggered, tec, &qPrefTechnicalDetails::set_calcndltts);
	connect(ui.profDeco,           &QAction::triggered, tec, &qPrefTechnicalDetails::set_decoinfo);
	connect(ui.profHR,             &QAction::triggered, tec, &qPrefTechnicalDetails::set_hrgraph);
	connect(ui.profRuler,          &QAction::triggered, tec, &qPrefTechnicalDetails::set_rulergraph);
	connect(ui.profSAC,            &QAction::triggered, tec, &qPrefTechnicalDetails::set_show_sac);
	connect(ui.profScaled,         &QAction::triggered, tec, &qPrefTechnicalDetails::set_zoomed_plot);
	connect(ui.profTogglePicture,  &QAction::triggered, tec, &qPrefTechnicalDetails::set_show_pictures_in_profile);
	connect(ui.profTankbar,        &QAction::triggered, tec, &qPrefTechnicalDetails::set_tankbar);
	connect(ui.profTissues,        &QAction::triggered, tec, &qPrefTechnicalDetails::set_percentagegraph);
	connect(ui.profInfobox,        &QAction::triggered, tec, &qPrefTechnicalDetails::set_infobox);

	connect(ui.profTissues,        &QAction::triggered, this, &ProfileWidget::unsetProfHR);
	connect(ui.profHR,             &QAction::triggered, this, &ProfileWidget::unsetProfTissues);

	auto pp_gas = qPrefPartialPressureGas::instance();
	connect(ui.profPhe, &QAction::triggered, pp_gas, &qPrefPartialPressureGas::set_phe);
	connect(ui.profPn2, &QAction::triggered, pp_gas, &qPrefPartialPressureGas::set_pn2);
	connect(ui.profPO2, &QAction::triggered, pp_gas, &qPrefPartialPressureGas::set_po2);

	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &ProfileWidget::divesChanged);

	ui.profCalcAllTissues->setChecked(qPrefTechnicalDetails::calcalltissues());
	ui.profCalcCeiling->setChecked(qPrefTechnicalDetails::calcceiling());
	ui.profDcCeiling->setChecked(qPrefTechnicalDetails::dcceiling());
	ui.profEad->setChecked(qPrefTechnicalDetails::ead());
	ui.profIncrement3m->setChecked(qPrefTechnicalDetails::calcceiling3m());
	ui.profMod->setChecked(qPrefTechnicalDetails::mod());
	ui.profNdl_tts->setChecked(qPrefTechnicalDetails::calcndltts());
	ui.profDeco->setChecked(qPrefTechnicalDetails::decoinfo());
	ui.profPhe->setChecked(pp_gas->phe());
	ui.profPn2->setChecked(pp_gas->pn2());
	ui.profPO2->setChecked(pp_gas->po2());
	ui.profHR->setChecked(qPrefTechnicalDetails::hrgraph());
	ui.profRuler->setChecked(qPrefTechnicalDetails::rulergraph());
	ui.profSAC->setChecked(qPrefTechnicalDetails::show_sac());
	ui.profTogglePicture->setChecked(qPrefTechnicalDetails::show_pictures_in_profile());
	ui.profTankbar->setChecked(qPrefTechnicalDetails::tankbar());
	ui.profTissues->setChecked(qPrefTechnicalDetails::percentagegraph());
	ui.profScaled->setChecked(qPrefTechnicalDetails::zoomed_plot());
	ui.profInfobox->setChecked(qPrefTechnicalDetails::infobox());

	//connect(&diveListNotifier, &DiveListNotifier::settingsChanged, view.get(), &ProfileWidget2::settingsChanged);
	//connect(view.get(), &ProfileWidget2::stopAdded, this, &ProfileWidget::stopAdded);
	//connect(view.get(), &ProfileWidget2::stopRemoved, this, &ProfileWidget::stopRemoved);
	//connect(view.get(), &ProfileWidget2::stopMoved, this, &ProfileWidget::stopMoved);
}

ProfileWidget::~ProfileWidget()
{
}

// hack around the Qt6 bug where the QML object gets destroyed and recreated
ProfileView *ProfileWidget::getView()
{
	ProfileView *view = qobject_cast<ProfileView *>(viewWidget->rootObject());
	if (!view)
		qWarning("Oops. The root of the StatsView is not a StatsView.");
	if (view) {
		// try to prevent the JS garbage collection from freeing the object
		// this appears to fail with Qt6 which is why we still look up the
		// object from the rootObject
		viewWidget->engine()->setObjectOwnership(view, QQmlEngine::CppOwnership);
		view->setParent(this);
		view->setVisible(isVisible()); // Synchronize visibility of widget and QtQuick-view.
	}
	return view;
}

void ProfileWidget::setEnabledToolbar(bool enabled)
{
	for (QAction *b: toolbarActions)
		b->setEnabled(enabled);
}

void ProfileWidget::setDive(const struct dive *d)
{
	stack->setCurrentIndex(1); // show profile

	bool freeDiveMode = d->dc.divemode == FREEDIVE;
	ui.profCalcCeiling->setDisabled(freeDiveMode);
	ui.profCalcCeiling->setDisabled(freeDiveMode);
	ui.profCalcAllTissues ->setDisabled(freeDiveMode);
	ui.profIncrement3m->setDisabled(freeDiveMode);
	ui.profDcCeiling->setDisabled(freeDiveMode);
	ui.profPhe->setDisabled(freeDiveMode);
	ui.profPn2->setDisabled(freeDiveMode); //TODO is the same as scuba?
	ui.profPO2->setDisabled(freeDiveMode); //TODO is the same as scuba?
	ui.profTankbar->setDisabled(freeDiveMode);
	ui.profMod->setDisabled(freeDiveMode);
	ui.profNdl_tts->setDisabled(freeDiveMode);
	ui.profDeco->setDisabled(freeDiveMode);
	ui.profEad->setDisabled(freeDiveMode);
	ui.profSAC->setDisabled(freeDiveMode);
	ui.profTissues->setDisabled(freeDiveMode);

	ui.profRuler->setDisabled(false);
	ui.profScaled->setDisabled(false); // measuring and scaling
	ui.profTogglePicture->setDisabled(false);
	ui.profHR->setDisabled(false);
	ui.profInfobox->setDisabled(false);
}

void ProfileWidget::plotCurrentDive()
{
	plotDive(d, dc);
}

void ProfileWidget::plotDive(dive *dIn, int dcIn)
{
	d = dIn;

	if (dcIn >= 0)
		dc = dcIn;

	// The following is valid because number_of_computers is always at least 1.
	if (d)
		dc = std::min(dc, (int)number_of_computers(current_dive) - 1);

	// Exit edit mode if the dive changed
	if (editedDive && (originalDive != d || editedDc != dc))
		exitEditMode();

	// If this is a manually added dive and we are not in the planner
	// or already editing the dive, switch to edit mode.
	if (d && !editedDive &&
	    DivePlannerPointsModel::instance()->currentMode() == DivePlannerPointsModel::NOTHING) {
		struct divecomputer *comp = get_dive_dc(d, dc);
		if (comp && is_manually_added_dc(comp) && comp->samples)
			editDive();
	}

	auto view = getView();
	setEnabledToolbar(d != nullptr);
	if (editedDive) {
		view->plotDive(editedDive.get(), editedDc);
		setDive(editedDive.get());
	} else if (d) {
		//view->setProfileState(d, dc);
		view->resetZoom(); // when switching dive, reset the zoomLevel
		view->plotDive(d, dc);
		setDive(d);
	} else {
		view->clear();
		stack->setCurrentIndex(0);
	}
}

void ProfileWidget::nextDC()
{
	rotateDC(1);
}

void ProfileWidget::prevDC()
{
	rotateDC(-1);
}

void ProfileWidget::rotateDC(int dir)
{
	if (!d)
		return;
	int numDC = number_of_computers(d);
	int newDC = (dc + dir) % numDC;
	if (newDC < 0)
		newDC += numDC;
	if (newDC == dc)
		return;

	plotDive(d, newDC);
}

void ProfileWidget::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If the current dive is not in list of changed dives, do nothing.
	// Only if duration or depth changed, the profile needs to be replotted.
	// Also, if we are currently placing a command, don't do anything.
	// Note that we cannot use Command::placingCommand(), because placing
	// a depth or time change on the maintab requires an update.
	if (!d || !dives.contains(d) || !(field.duration || field.depth) || placingCommand)
		return;

	// If we're editing the current dive and not currently
	// placing a command, we have to update the edited dive.
	if (editedDive) {
		copy_dive(d, editedDive.get());
		// TODO: Holy moly that function sends too many signals. Fix it!
		DivePlannerPointsModel::instance()->loadFromDive(editedDive.get(), editedDc);
	}

	plotCurrentDive();
}

void ProfileWidget::setPlanState(const struct dive *d, int dc)
{
	exitEditMode();
	//view->setPlanState(d, dc);
	setDive(d);
}

void ProfileWidget::unsetProfHR()
{
	ui.profHR->setChecked(false);
	qPrefTechnicalDetails::set_hrgraph(false);
}

void ProfileWidget::unsetProfTissues()
{
	ui.profTissues->setChecked(false);
	qPrefTechnicalDetails::set_percentagegraph(false);
}

void ProfileWidget::editDive()
{
	editedDive.reset(alloc_dive());
	editedDc = dc;
	copy_dive(d, editedDive.get()); // Work on a copy of the dive
	originalDive = d;
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::ADD);
	DivePlannerPointsModel::instance()->loadFromDive(editedDive.get(), editedDc);
	//view->setEditState(editedDive.get(), editedDc);
}

void ProfileWidget::exitEditMode()
{
	if (!editedDive)
		return;
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::NOTHING);
	//view->setProfileState(d, dc); // switch back to original dive before erasing the copy.
	editedDive.reset();
	originalDive = nullptr;
}

// Update depths of edited dive
static void calcDepth(dive &d, int dcNr)
{
	d.maxdepth.mm = get_dive_dc(&d, dcNr)->maxdepth.mm = 0;
	fixup_dive(&d);
}

// Silly RAII-variable setter class: reset variable when going out of scope.
template <typename T>
struct Setter {
	T &var, old;
	Setter(T &var, T value) : var(var), old(var) {
		var = value;
	}
	~Setter() {
		var = old;
	}
};

void ProfileWidget::stopAdded()
{
	if (!editedDive)
		return;
	calcDepth(*editedDive, editedDc);
	Setter s(placingCommand, true);
	Command::editProfile(editedDive.get(), editedDc, Command::EditProfileType::ADD, 0);
}

void ProfileWidget::stopRemoved(int count)
{
	if (!editedDive)
		return;
	calcDepth(*editedDive, editedDc);
	Setter s(placingCommand, true);
	Command::editProfile(editedDive.get(), editedDc, Command::EditProfileType::REMOVE, count);
}

void ProfileWidget::stopMoved(int count)
{
	if (!editedDive)
		return;
	calcDepth(*editedDive, editedDc);
	Setter s(placingCommand, true);
	Command::editProfile(editedDive.get(), editedDc, Command::EditProfileType::MOVE, count);
}
