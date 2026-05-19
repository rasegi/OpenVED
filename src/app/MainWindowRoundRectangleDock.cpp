#include "MainWindow.h"

#include "QVedWidget.h"
#include "vec_edit_cad.h"
#include "vec_font.h"
#include "vec_model.h"
#include "vec_text.h"
#include "voc_bspline_controlpoint.h"
#include "voc_polycurve.h"
#include "voc_roundrectangle_notrotated.h"
#include "voc_vecframetext_empty.h"
#include "voc_vectext.h"
#include "vom_modify_curve_attribute.h"
#include "vom_rotate_object_activepoint.h"
#include "vop_manager.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleValidator>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <Qt>

#include <cstdio>

void MainWindow::createRoundRectangleDock() {
    roundRectangleDock_ = new QDockWidget(QStringLiteral("Abgerundetes Rechteck"), this);
    roundRectangleDock_->setObjectName(QStringLiteral("RoundRectangleDock"));
    roundRectangleDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* panel = new QWidget(roundRectangleDock_);
    auto* rootLayout = new QVBoxLayout(panel);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(6);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch();

    auto* applyButton = new QToolButton(panel);
    applyButton->setIcon(QIcon(QStringLiteral(":/ved/toolbar/apply.bmp")));
    applyButton->setToolTip(QStringLiteral("Apply"));
    buttonLayout->addWidget(applyButton);

    auto* cancelButton = new QToolButton(panel);
    cancelButton->setIcon(QIcon(QStringLiteral(":/ved/toolbar/delete.bmp")));
    cancelButton->setToolTip(QStringLiteral("Cancel"));
    buttonLayout->addWidget(cancelButton);
    rootLayout->addLayout(buttonLayout);

    roundRectangleRadiusEdit_ = new QLineEdit(panel);
    auto* radiusValidator = new QDoubleValidator(0.0, 1000000000.0, 6, roundRectangleRadiusEdit_);
    radiusValidator->setNotation(QDoubleValidator::StandardNotation);
    roundRectangleRadiusEdit_->setValidator(radiusValidator);
    roundRectangleRadiusEdit_->setFixedWidth(88);

    roundRectangleResolutionEdit_ = new QLineEdit(panel);
    roundRectangleResolutionEdit_->setValidator(new QIntValidator(1, 9999, roundRectangleResolutionEdit_));
    roundRectangleResolutionEdit_->setFixedWidth(64);

    roundRectangleWidthLabel_ = new QLabel(QStringLiteral("-"), panel);
    roundRectangleHeightLabel_ = new QLabel(QStringLiteral("-"), panel);
    roundRectangleShowPolygonCheck_ = new QCheckBox(QStringLiteral("show polygon"), panel);
    roundRectangleShowControlsCheck_ = new QCheckBox(QStringLiteral("show controls"), panel);

    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(QStringLiteral("radius:"), roundRectangleRadiusEdit_);
    formLayout->addRow(QStringLiteral("resolution:"), roundRectangleResolutionEdit_);
    formLayout->addRow(QStringLiteral("width:"), roundRectangleWidthLabel_);
    formLayout->addRow(QStringLiteral("height:"), roundRectangleHeightLabel_);
    rootLayout->addLayout(formLayout);
    rootLayout->addWidget(roundRectangleShowPolygonCheck_);
    rootLayout->addWidget(roundRectangleShowControlsCheck_);
    rootLayout->addStretch();

    roundRectangleDock_->setWidget(panel);
    roundRectangleDock_->hide();
    addDockWidget(Qt::RightDockWidgetArea, roundRectangleDock_);

    connect(applyButton, &QToolButton::clicked, this, &MainWindow::applyRoundRectangleSettings);
    connect(cancelButton, &QToolButton::clicked, this, &MainWindow::cancelRoundRectangleDockOperation);
    connect(roundRectangleRadiusEdit_, &QLineEdit::returnPressed, this, &MainWindow::applyRoundRectangleSettings);
    connect(roundRectangleResolutionEdit_, &QLineEdit::returnPressed, this, &MainWindow::applyRoundRectangleSettings);
    connect(roundRectangleDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible && operationManager_ &&
            operationManager_->GetAktiveOperationType() == VOC_ROUND_RECTANGLE_NOTROTATED) {
            cancelRoundRectangleDockOperation();
        }
    });
}

void MainWindow::showRoundRectangleDock() {
    if (!roundRectangleDock_ || !operationManager_ ||
        operationManager_->GetAktiveOperationType() != VOC_ROUND_RECTANGLE_NOTROTATED) {
        return;
    }

    roundRectangleRadiusEdit_->setText(QStringLiteral("0"));
    roundRectangleResolutionEdit_->setText(QStringLiteral("5"));
    roundRectangleShowPolygonCheck_->setChecked(false);
    roundRectangleShowControlsCheck_->setChecked(false);
    roundRectangleWidthLabel_->setText(QStringLiteral("-"));
    roundRectangleHeightLabel_->setText(QStringLiteral("-"));

    if (auto* extVar = dynamic_cast<TDVOCRoundRectNotRotExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        roundRectangleRadiusEdit_->setText(QString::number(extVar->GetRadius(), 'f', 2));
        roundRectangleResolutionEdit_->setText(QString::number(extVar->GetResolution()));
        roundRectangleShowPolygonCheck_->setChecked(extVar->GetShowPolygon());
        roundRectangleShowControlsCheck_->setChecked(extVar->GetShowControls());
        refreshRoundRectangleDockFromExtVar();
    }

    roundRectangleDock_->show();
    roundRectangleDock_->raise();
    applyRoundRectangleSettings();
}

void MainWindow::applyRoundRectangleSettings() {
    if (!operationManager_ || operationManager_->GetAktiveOperationType() != VOC_ROUND_RECTANGLE_NOTROTATED) {
        return;
    }

    auto* extVar = dynamic_cast<TDVOCRoundRectNotRotExtVar*>(operationManager_->GetActiveVOPExtVariables());
    if (!extVar) {
        return;
    }

    bool radiusOk = false;
    const double radius = roundRectangleRadiusEdit_->text().toDouble(&radiusOk);
    bool resolutionOk = false;
    const int resolution = roundRectangleResolutionEdit_->text().toInt(&resolutionOk);

    if (!radiusOk) {
        statusBar()->showMessage(QStringLiteral("VOC_ROUND_RECTANGLE_NOTROTATED invalid radius"), 1500);
        return;
    }

    extVar->SetRectangularLock(true);
    extVar->SetRadius(radius);
    extVar->SetResolution(resolutionOk && resolution > 0 ? static_cast<unsigned int>(resolution) : 5);
    extVar->SetShowControls(roundRectangleShowControlsCheck_->isChecked());
    extVar->SetShowPolygon(roundRectangleShowPolygonCheck_->isChecked());
    extVar->SendUpdateToParrentOP();
    refreshRoundRectangleDockFromExtVar();
    statusBar()->showMessage(QStringLiteral("VOC_ROUND_RECTANGLE_NOTROTATED settings applied"), 1500);
}

void MainWindow::cancelRoundRectangleDockOperation() {
    if (!operationManager_ || operationManager_->GetAktiveOperationType() != VOC_ROUND_RECTANGLE_NOTROTATED) {
        return;
    }

    if (editor_) {
        editor_->SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
    }
    if (selectAction_) {
        selectAction_->setChecked(true);
    }
    deactivateViewTools();
    canvas_->setInteractionMode(QVedWidget::InteractionMode::SelectObject);
    setActiveOperationStatus(QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"));
    if (roundRectangleDock_->isVisible()) {
        roundRectangleDock_->hide();
    }
    statusBar()->showMessage(QStringLiteral("VOC_ROUND_RECTANGLE_NOTROTATED canceled"), 1500);
}

void MainWindow::refreshRoundRectangleDockFromExtVar() {
    if (!roundRectangleDock_ || !operationManager_ ||
        operationManager_->GetAktiveOperationType() != VOC_ROUND_RECTANGLE_NOTROTATED) {
        return;
    }

    auto* extVar = dynamic_cast<TDVOCRoundRectNotRotExtVar*>(operationManager_->GetActiveVOPExtVariables());
    if (!extVar) {
        return;
    }

    roundRectangleWidthLabel_->setText(QString::number(extVar->GetRectWidth(), 'f', 2));
    roundRectangleHeightLabel_->setText(QString::number(extVar->GetRectHeight(), 'f', 2));
}

void MainWindow::hideRoundRectangleDockIfInactive() {
    if (!roundRectangleDock_ || !operationManager_) {
        return;
    }

    if (operationManager_->GetAktiveOperationType() == VOC_ROUND_RECTANGLE_NOTROTATED) {
        return;
    }

    roundRectangleDock_->hide();
}

