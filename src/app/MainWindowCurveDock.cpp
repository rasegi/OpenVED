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

void MainWindow::createCurveDock() {
    curveDock_ = new QDockWidget(QStringLiteral("PolyCurve zeichnen"), this);
    curveDock_->setObjectName(QStringLiteral("CurveDock"));
    curveDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* panel = new QWidget(curveDock_);
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

    curveShowPolygonCheck_ = new QCheckBox(QStringLiteral("show polygon"), panel);
    curveShowControlsCheck_ = new QCheckBox(QStringLiteral("show controls"), panel);
    curveCopyCheck_ = new QCheckBox(QStringLiteral("copy"), panel);
    curveDegreeEdit_ = new QLineEdit(panel);
    curveDegreeEdit_->setValidator(new QIntValidator(1, 20, curveDegreeEdit_));
    curveDegreeEdit_->setFixedWidth(64);
    curveResolutionEdit_ = new QLineEdit(panel);
    curveResolutionEdit_->setValidator(new QIntValidator(1, 9999, curveResolutionEdit_));
    curveResolutionEdit_->setFixedWidth(64);

    rootLayout->addWidget(curveShowPolygonCheck_);
    rootLayout->addWidget(curveShowControlsCheck_);
    rootLayout->addWidget(curveCopyCheck_);

    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    curveDegreeLabel_ = new QLabel(QStringLiteral("degree:"), panel);
    formLayout->addRow(curveDegreeLabel_, curveDegreeEdit_);
    formLayout->addRow(QStringLiteral("resolution:"), curveResolutionEdit_);
    rootLayout->addLayout(formLayout);
    rootLayout->addStretch();

    curveDock_->setWidget(panel);
    curveDock_->hide();
    addDockWidget(Qt::RightDockWidgetArea, curveDock_);

    connect(applyButton, &QToolButton::clicked, this, &MainWindow::applyPolyCurveSettings);
    connect(cancelButton, &QToolButton::clicked, this, &MainWindow::cancelCurveDockOperation);
    connect(curveDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible && operationManager_ &&
            (operationManager_->GetAktiveOperationType() == VOC_POLYCURVE ||
             operationManager_->GetAktiveOperationType() == VOC_BSPLINE_CONTROLPOINT ||
             operationManager_->GetAktiveOperationType() == VOM_MODIFY_CURVE_ATTRIBUTE)) {
            cancelCurveDockOperation();
        }
    });
}

void MainWindow::showPolyCurveDock() {
    if (!curveDock_ || !operationManager_) {
        return;
    }

    const TDViewOperationEnum activeOperation = operationManager_->GetAktiveOperationType();
    const bool isBSPLine = activeOperation == VOC_BSPLINE_CONTROLPOINT;
    const bool isModifyCurveAttribute = activeOperation == VOM_MODIFY_CURVE_ATTRIBUTE;
    if (activeOperation != VOC_POLYCURVE && !isBSPLine && !isModifyCurveAttribute) {
        return;
    }

    curveDock_->setWindowTitle(isBSPLine ? QStringLiteral("BSPLine zeichnen") : QStringLiteral("PolyCurve zeichnen"));
    curveCopyCheck_->setVisible(isModifyCurveAttribute);
    curveCopyCheck_->setChecked(false);
    curveDegreeEdit_->setEnabled(isBSPLine);
    curveDegreeLabel_->setEnabled(isBSPLine);
    curveDegreeEdit_->setText(QStringLiteral("2"));
    curveResolutionEdit_->setText(isBSPLine ? QStringLiteral("200") : QStringLiteral("5"));
    curveShowPolygonCheck_->setChecked(true);
    curveShowControlsCheck_->setChecked(true);

    if (auto* extVar = dynamic_cast<TDVOCPolyCurveExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        curveResolutionEdit_->setText(QString::number(extVar->GetResolution()));
        curveShowPolygonCheck_->setChecked(extVar->GetShowPolygon());
        curveShowControlsCheck_->setChecked(extVar->GetShowControls());
    }
    if (auto* extVar = dynamic_cast<TDVOMModifyCurveAttributeExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        const bool modifyBSPLine = extVar->GetObjectType() == VECOBJ_BSPLINE;
        curveDock_->setWindowTitle(modifyBSPLine ? QStringLiteral("BSPLine ändern") : QStringLiteral("PolyCurve ändern"));
        curveDegreeEdit_->setEnabled(modifyBSPLine);
        curveDegreeLabel_->setEnabled(modifyBSPLine);
        curveDegreeEdit_->setText(QString::number(extVar->GetDegree()));
        curveResolutionEdit_->setText(QString::number(extVar->GetResolution()));
        curveShowPolygonCheck_->setChecked(extVar->GetShowPolygon());
        curveShowControlsCheck_->setChecked(extVar->GetShowControls());
        curveCopyCheck_->setChecked(extVar->GetCopyFlag());
    }
    if (auto* extVar = dynamic_cast<TDVOCBSPLineControlPtExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        curveDegreeEdit_->setText(QString::number(extVar->GetDegree()));
        if (extVar->GetResolution() != 50) {
            curveResolutionEdit_->setText(QString::number(extVar->GetResolution()));
        }
        curveShowPolygonCheck_->setChecked(extVar->GetShowPolygon());
        curveShowControlsCheck_->setChecked(extVar->GetShowControls());
    }

    curveDock_->show();
    curveDock_->raise();
    if (isBSPLine) {
        applyPolyCurveSettings();
    }
}

void MainWindow::applyPolyCurveSettings() {
    if (!operationManager_) {
        return;
    }

    const TDViewOperationEnum activeOperation = operationManager_->GetAktiveOperationType();
    if (activeOperation != VOC_POLYCURVE &&
        activeOperation != VOC_BSPLINE_CONTROLPOINT &&
        activeOperation != VOM_MODIFY_CURVE_ATTRIBUTE) {
        return;
    }

    bool resolutionOk = false;
    const int resolution = curveResolutionEdit_->text().toInt(&resolutionOk);

    if (auto* extVar = dynamic_cast<TDVOCPolyCurveExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        extVar->SetResolution(resolutionOk && resolution > 0 ? static_cast<unsigned int>(resolution) : 5);
        extVar->SetShowControls(curveShowControlsCheck_->isChecked());
        extVar->SetShowPolygon(curveShowPolygonCheck_->isChecked());
        extVar->SendUpdateToParrentOP();
        statusBar()->showMessage(QStringLiteral("VOC_POLYCURVE settings applied"), 1500);
        return;
    }

    if (auto* extVar = dynamic_cast<TDVOCBSPLineControlPtExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        bool degreeOk = false;
        const int degree = curveDegreeEdit_->text().toInt(&degreeOk);
        extVar->SetDegree(degreeOk && degree > 0 ? degree : 2);
        extVar->SetResolution(resolutionOk && resolution > 0 ? static_cast<unsigned int>(resolution) : 200);
        extVar->SetShowControls(curveShowControlsCheck_->isChecked());
        extVar->SetShowPolygon(curveShowPolygonCheck_->isChecked());
        extVar->SendUpdateToParrentOP();
        statusBar()->showMessage(QStringLiteral("VOC_BSPLINE_CONTROLPOINT settings applied"), 1500);
        return;
    }

    if (auto* extVar = dynamic_cast<TDVOMModifyCurveAttributeExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        bool degreeOk = false;
        const int degree = curveDegreeEdit_->text().toInt(&degreeOk);
        extVar->SetDegree(degreeOk && degree > 0 ? degree : 2);
        extVar->SetResolution(resolutionOk && resolution > 0 ? static_cast<unsigned int>(resolution) : 5);
        extVar->SetShowControls(curveShowControlsCheck_->isChecked());
        extVar->SetShowPolygon(curveShowPolygonCheck_->isChecked());
        extVar->SetCopyFlag(curveCopyCheck_->isChecked());
        extVar->SendUpdateToParrentOP();
        statusBar()->showMessage(QStringLiteral("VOM_MODIFY_CURVE_ATTRIBUTE settings applied"), 1500);
    }
}

void MainWindow::cancelCurveDockOperation() {
    if (!operationManager_ ||
        (operationManager_->GetAktiveOperationType() != VOC_POLYCURVE &&
         operationManager_->GetAktiveOperationType() != VOC_BSPLINE_CONTROLPOINT &&
         operationManager_->GetAktiveOperationType() != VOM_MODIFY_CURVE_ATTRIBUTE)) {
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
    if (curveDock_->isVisible()) {
        curveDock_->hide();
    }
    statusBar()->showMessage(QStringLiteral("Curve operation canceled"), 1500);
}

void MainWindow::hideCurveDockIfInactive() {
    if (!curveDock_ || !operationManager_) {
        return;
    }

    const TDViewOperationEnum activeOperation = operationManager_->GetAktiveOperationType();
    if (activeOperation == VOC_POLYCURVE ||
        activeOperation == VOC_BSPLINE_CONTROLPOINT ||
        activeOperation == VOM_MODIFY_CURVE_ATTRIBUTE) {
        return;
    }

    curveDock_->hide();
}

