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

void MainWindow::createRotateDock() {
    rotateDock_ = new QDockWidget(QStringLiteral("Objekt drehen"), this);
    rotateDock_->setObjectName(QStringLiteral("RotateDock"));
    rotateDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* panel = new QWidget(rotateDock_);
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

    rotateAngleEdit_ = new QLineEdit(panel);
    auto* angleValidator = new QDoubleValidator(-360000.0, 360000.0, 6, rotateAngleEdit_);
    angleValidator->setNotation(QDoubleValidator::StandardNotation);
    rotateAngleEdit_->setValidator(angleValidator);
    rotateAngleEdit_->setFixedWidth(88);

    rotateCopyCheck_ = new QCheckBox(QStringLiteral("copy"), panel);
    rotateSelectCopyCheck_ = new QCheckBox(QStringLiteral("select copy"), panel);
    rotateSelectCopyCheck_->setEnabled(false);
    rotateObjectCountLabel_ = new QLabel(QStringLiteral("-"), panel);
    rotatePivotLabel_ = new QLabel(QStringLiteral("X: -  Y: -"), panel);

    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(QStringLiteral("angle:"), rotateAngleEdit_);
    formLayout->addRow(QStringLiteral("objects:"), rotateObjectCountLabel_);
    formLayout->addRow(QStringLiteral("pivot:"), rotatePivotLabel_);
    rootLayout->addLayout(formLayout);
    rootLayout->addWidget(rotateCopyCheck_);
    rootLayout->addWidget(rotateSelectCopyCheck_);
    rootLayout->addStretch();

    rotateDock_->setWidget(panel);
    rotateDock_->hide();
    addDockWidget(Qt::RightDockWidgetArea, rotateDock_);

    connect(applyButton, &QToolButton::clicked, this, &MainWindow::applyRotateSettings);
    connect(cancelButton, &QToolButton::clicked, this, &MainWindow::cancelRotateDockOperation);
    connect(rotateAngleEdit_, &QLineEdit::returnPressed, this, &MainWindow::applyRotateSettings);
    connect(rotateCopyCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        rotateSelectCopyCheck_->setEnabled(checked);
        if (!checked) {
            rotateSelectCopyCheck_->setChecked(false);
        }
        updateRotateExtVarFromDock(false);
    });
    connect(rotateSelectCopyCheck_, &QCheckBox::toggled, this, [this](bool) {
        updateRotateExtVarFromDock(false);
    });
    connect(rotateDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible && operationManager_ && operationManager_->GetAktiveOperationType() == VOM_ROTATE_OBJECT_ACTIVEPOINT) {
            cancelRotateDockOperation();
        }
    });
}

void MainWindow::showRotateDock() {
    if (!rotateDock_ || !operationManager_ || operationManager_->GetAktiveOperationType() != VOM_ROTATE_OBJECT_ACTIVEPOINT) {
        return;
    }

    rotateDock_->setWindowTitle(QStringLiteral("Objekt drehen"));
    rotateAngleEdit_->setText(QStringLiteral("45"));
    rotateCopyCheck_->setChecked(false);
    rotateSelectCopyCheck_->setChecked(false);
    rotateSelectCopyCheck_->setEnabled(false);
    if (auto* extVar = dynamic_cast<TDVOMRotateObjectActPtExtVar*>(operationManager_->GetActiveVOPExtVariables())) {
        if (extVar->GetInitializedFlag()) {
            rotateAngleEdit_->setText(QString::number(-extVar->GetAngle(), 'f', 2));
        }
        rotateCopyCheck_->setChecked(extVar->GetCopyFlag());
        rotateSelectCopyCheck_->setEnabled(extVar->GetCopyFlag());
        rotateSelectCopyCheck_->setChecked(extVar->GetCopyFlag() && extVar->GetSelectFlag());
    }
    refreshRotateDockFromExtVar();

    rotateDock_->show();
    rotateDock_->raise();
    updateRotateExtVarFromDock(false);
}

void MainWindow::applyRotateSettings() {
    updateRotateExtVarFromDock(true);
}

void MainWindow::cancelRotateDockOperation() {
    if (!operationManager_ || operationManager_->GetAktiveOperationType() != VOM_ROTATE_OBJECT_ACTIVEPOINT) {
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
    if (rotateDock_->isVisible()) {
        rotateDock_->hide();
    }
    statusBar()->showMessage(QStringLiteral("VOM_ROTATE_OBJECT_ACTIVEPOINT canceled"), 1500);
}

void MainWindow::refreshRotateDockFromExtVar() {
    if (!rotateDock_ || !operationManager_ || operationManager_->GetAktiveOperationType() != VOM_ROTATE_OBJECT_ACTIVEPOINT) {
        return;
    }

    auto* extVar = dynamic_cast<TDVOMRotateObjectActPtExtVar*>(operationManager_->GetActiveVOPExtVariables());
    if (!extVar) {
        return;
    }

    rotateObjectCountLabel_->setText(QString::number(extVar->GetNumberOfObjects()));

    const TDMatPoint pivot = extVar->GetPivotPoint();
    rotatePivotLabel_->setText(
        QStringLiteral("X: %1  Y: %2")
            .arg(pivot.x, 0, 'f', 2)
            .arg(pivot.y, 0, 'f', 2));
}

bool MainWindow::updateRotateExtVarFromDock(bool showMessage) {
    if (!operationManager_ || operationManager_->GetAktiveOperationType() != VOM_ROTATE_OBJECT_ACTIVEPOINT) {
        return false;
    }

    auto* extVar = dynamic_cast<TDVOMRotateObjectActPtExtVar*>(operationManager_->GetActiveVOPExtVariables());
    if (!extVar) {
        return false;
    }

    bool ok = false;
    const double displayAngle = rotateAngleEdit_->text().toDouble(&ok);
    if (!ok) {
        if (showMessage) {
            statusBar()->showMessage(QStringLiteral("VOM_ROTATE_OBJECT_ACTIVEPOINT invalid angle"), 1500);
        }
        return false;
    }

    extVar->SetAngle(-displayAngle);
    extVar->SetCopyFlag(rotateCopyCheck_->isChecked());
    extVar->SetSelectFlag(rotateCopyCheck_->isChecked() && rotateSelectCopyCheck_->isChecked());
    extVar->SendUpdateToParrentOP();
    refreshRotateDockFromExtVar();
    if (showMessage) {
        statusBar()->showMessage(QStringLiteral("VOM_ROTATE_OBJECT_ACTIVEPOINT settings applied"), 1500);
    }
    return true;
}

