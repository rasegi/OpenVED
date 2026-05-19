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
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <Qt>

#include <cstdio>

namespace {
QIcon makeMouseToleranceCrossIcon() {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.fillRect(QRect(0, 0, 16, 16), QColor(245, 245, 242));
    painter.setPen(QPen(QColor(190, 190, 190), 1));
    for (int pos : {3, 7, 11, 15}) {
        painter.drawLine(pos, 0, pos, 15);
        painter.drawLine(0, pos, 15, pos);
    }
    painter.setPen(QPen(QColor(20, 20, 20), 1));
    painter.drawLine(2, 8, 14, 8);
    painter.drawLine(8, 2, 8, 14);
    painter.drawPoint(8, 8);
    return QIcon(pixmap);
}

QIcon makeRulersIcon() {
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.fillRect(QRect(0, 0, 16, 16), QColor(250, 250, 248));
    painter.fillRect(QRect(0, 0, 16, 4), QColor(228, 228, 224));
    painter.fillRect(QRect(0, 0, 4, 16), QColor(228, 228, 224));
    painter.setPen(QPen(QColor(60, 60, 55), 1));
    painter.drawLine(0, 4, 15, 4);
    painter.drawLine(4, 0, 4, 15);
    for (int pos : {6, 10, 14}) {
        painter.drawLine(pos, 4, pos, 1);
        painter.drawLine(4, pos, 1, pos);
    }
    painter.drawLine(8, 4, 8, 0);
    painter.drawLine(4, 8, 0, 8);
    return QIcon(pixmap);
}

} // namespace

void MainWindow::createToolBars() {
    setIconSize(QSize(16, 16));

    auto* fileToolbar = new QToolBar(QStringLiteral("File"), this);
    fileToolbar->setObjectName(QStringLiteral("FileToolbar"));
    addToolBar(Qt::TopToolBarArea, fileToolbar);
    auto* newAction = addToolAction(fileToolbar, QStringLiteral("New"), QStringLiteral("new.bmp"), false, QStringLiteral("New"));
    newAction->setProperty("implemented", true);
    newAction->setEnabled(true);
    connect(newAction, &QAction::triggered, this, [this] {
        newDocument();
    });
    auto* openAction = addToolAction(fileToolbar, QStringLiteral("Open"), QStringLiteral("open.bmp"), false, QStringLiteral("Open"));
    openAction->setProperty("implemented", true);
    openAction->setEnabled(true);
    connect(openAction, &QAction::triggered, this, [this] {
        openDocument();
    });
    auto* saveAction = addToolAction(fileToolbar, QStringLiteral("Save"), QStringLiteral("save.bmp"), false, QStringLiteral("Save"));
    saveAction->setProperty("implemented", true);
    saveAction->setEnabled(true);
    connect(saveAction, &QAction::triggered, this, [this] {
        saveDocument();
    });
    fileToolbar->addSeparator();
    if (undoAction_) {
        undoAction_->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
        undoAction_->setToolTip(QStringLiteral("Undo"));
        undoAction_->setStatusTip(QStringLiteral("Undo"));
        fileToolbar->addAction(undoAction_);
    }
    if (redoAction_) {
        redoAction_->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
        redoAction_->setToolTip(QStringLiteral("Redo"));
        redoAction_->setStatusTip(QStringLiteral("Redo"));
        fileToolbar->addAction(redoAction_);
    }
    fileToolbar->addSeparator();
    cutAction_ = addToolAction(fileToolbar, QStringLiteral("Cut"), QStringLiteral("cut.bmp"), false, QStringLiteral("Cut"));
    cutAction_->setProperty("implemented", true);
    cutAction_->setShortcut(QKeySequence::Cut);
    cutAction_->setStatusTip(QStringLiteral("Cut"));
    connect(cutAction_, &QAction::triggered, this, [this] {
        if (operationManager_ && operationManager_->CutSelectedObjects()) {
            statusBar()->showMessage(QStringLiteral("Cut"), 1500);
            updateGroupCommandActions();
            updateModifyCommandActions();
        }
    });
    copyAction_ = addToolAction(fileToolbar, QStringLiteral("Copy"), QStringLiteral("copy.bmp"), false, QStringLiteral("Copy"));
    copyAction_->setProperty("implemented", true);
    copyAction_->setShortcut(QKeySequence::Copy);
    copyAction_->setStatusTip(QStringLiteral("Copy"));
    connect(copyAction_, &QAction::triggered, this, [this] {
        if (operationManager_ && operationManager_->CopySelectedObjects()) {
            statusBar()->showMessage(QStringLiteral("Copy"), 1500);
            updateClipboardCommandActions();
        }
    });
    pasteAction_ = addToolAction(fileToolbar, QStringLiteral("Paste"), QStringLiteral("paste.bmp"), false, QStringLiteral("Paste"));
    pasteAction_->setProperty("implemented", true);
    pasteAction_->setShortcut(QKeySequence::Paste);
    pasteAction_->setStatusTip(QStringLiteral("Paste"));
    connect(pasteAction_, &QAction::triggered, this, [this] {
        if (operationManager_ && operationManager_->PasteClipboardObjects()) {
            statusBar()->showMessage(QStringLiteral("Paste"), 1500);
            updateGroupCommandActions();
            updateModifyCommandActions();
        }
    });
    fileToolbar->addSeparator();
    addToolAction(fileToolbar, QStringLiteral("Print"), QStringLiteral("print.bmp"));
    addToolAction(fileToolbar, QStringLiteral("Preview"), QStringLiteral("preview.bmp"));

    addToolBarBreak(Qt::TopToolBarArea);
    auto* createToolbar = new QToolBar(QStringLiteral("Create"), this);
    createToolbar->setObjectName(QStringLiteral("CreateToolbar"));
    auto* createActionGroup = new QActionGroup(createToolbar);
    createActionGroup->setExclusive(true);
    addToolBar(Qt::TopToolBarArea, createToolbar);
    auto* lineAction = addToolAction(createToolbar, QStringLiteral("Line"), QStringLiteral("line.bmp"), true, QStringLiteral("VOC_LINE"));
    lineAction->setProperty("implemented", true);
    lineAction->setProperty("createOperation", true);
    lineAction->setEnabled(true);
    lineAction->setStatusTip(QStringLiteral("VOC_LINE"));
    createActionGroup->addAction(lineAction);
    connect(lineAction, &QAction::triggered, this, [this, lineAction](bool checked) {
        if (checked) {
            deactivateViewTools();
            if (editor_) {
                editor_->SetOperation(VOC_LINE);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::CreateLine);
            setActiveOperationStatus(QStringLiteral("VOC_LINE"));
            statusBar()->showMessage(QStringLiteral("VOC_LINE"), 1500);
        } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::CreateLine) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            setActiveOperationStatus(QString());
        }
    });
    auto addCreateOperationAction = [this, createToolbar, createActionGroup](
        const QString& text,
        const QString& iconName,
        TDViewOperationEnum operation,
        const QString& operationName) {
        auto* action = addToolAction(createToolbar, text, iconName, true, operationName);
        action->setProperty("implemented", true);
        action->setProperty("createOperation", true);
        action->setEnabled(true);
        action->setStatusTip(operationName);
        createActionGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, action, operation, operationName](bool checked) {
            if (checked) {
                deactivateViewTools();
                if (editor_) {
                    editor_->SetOperation(operation);
                }
                canvas_->setInteractionMode(QVedWidget::InteractionMode::CreateLine);
                setActiveOperationStatus(operationName);
                statusBar()->showMessage(operationName, 1500);
            } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::CreateLine) {
                canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
                setActiveOperationStatus(QString());
            }
        });
        return action;
    };
    addCreateOperationAction(QStringLiteral("Polygon"), QStringLiteral("polygon.bmp"), VOC_POLYGON_SMARTLINE, QStringLiteral("VOC_POLYGON_SMARTLINE"));
    auto* polyCurveAction = addCreateOperationAction(QStringLiteral("PolyCurve"), QStringLiteral("polycurve.bmp"), VOC_POLYCURVE, QStringLiteral("VOC_POLYCURVE"));
    connect(polyCurveAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            showPolyCurveDock();
        }
    });
    connect(polyCurveAction, &QAction::toggled, this, [this](bool checked) {
        if (!checked && curveDock_) {
            curveDock_->hide();
        }
    });
    addCreateOperationAction(QStringLiteral("bz"), QString(), VOC_BEZIERCURVE_CONTROLPOINT, QStringLiteral("VOC_BEZIERCURVE_CONTROLPOINT"));
    auto* bsplineAction = addCreateOperationAction(QStringLiteral("B-Spline"), QStringLiteral("bspline.bmp"), VOC_BSPLINE_CONTROLPOINT, QStringLiteral("VOC_BSPLINE_CONTROLPOINT"));
    connect(bsplineAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            showPolyCurveDock();
        }
    });
    connect(bsplineAction, &QAction::toggled, this, [this](bool checked) {
        if (!checked && curveDock_) {
            curveDock_->hide();
        }
    });
    createToolbar->addSeparator();
    addCreateOperationAction(QStringLiteral("Circle"), QStringLiteral("circle_radius.bmp"), VOC_CIRCLE_MIDPOINT, QStringLiteral("VOC_CIRCLE_MIDPOINT"));
    addCreateOperationAction(QStringLiteral("Circle 3 Points"), QStringLiteral("circle_3points.bmp"), VOC_CIRCLE_EDGE, QStringLiteral("VOC_CIRCLE_EDGE"));
    addCreateOperationAction(QStringLiteral("Circle Diameter"), QStringLiteral("circle_diameter.bmp"), VOC_CIRCLE_DIAMETER, QStringLiteral("VOC_CIRCLE_DIAMETER"));
    addCreateOperationAction(QStringLiteral("Circle Diagonal"), QStringLiteral("circle_square.bmp"), VOC_CIRCLE_DIAGONAL, QStringLiteral("VOC_CIRCLE_DIAGONAL"));
    createToolbar->addSeparator();
    addCreateOperationAction(QStringLiteral("Ellipse Orthogonal"), QStringLiteral("ellipse_orthogonal.bmp"), VOC_ELLIPSE_ORTHOGONAL, QStringLiteral("VOC_ELLIPSE_ORTHOGONAL"));
    addCreateOperationAction(QStringLiteral("Ellipse Rotated"), QStringLiteral("ellipse_rotated.bmp"), VOC_ELLIPSE_MIDPOINT, QStringLiteral("VOC_ELLIPSE_MIDPOINT"));
    addCreateOperationAction(QStringLiteral("Rectangle Orthogonal"), QStringLiteral("rectangle_orthogonal.bmp"), VOC_RECTANGLE_NOTROTATED, QStringLiteral("VOC_RECTANGLE_NOTROTATED"));
    addCreateOperationAction(QStringLiteral("Rectangle Rotated"), QStringLiteral("rectangle_rotated.bmp"), VOC_RECTANGLE_ROTATED, QStringLiteral("VOC_RECTANGLE_ROTATED"));
    auto* roundRectangleAction = addCreateOperationAction(QStringLiteral("Round Rectangle"), QStringLiteral("roundrect_orthogonal.bmp"), VOC_ROUND_RECTANGLE_NOTROTATED, QStringLiteral("VOC_ROUND_RECTANGLE_NOTROTATED"));
    connect(roundRectangleAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            showRoundRectangleDock();
        }
    });
    connect(roundRectangleAction, &QAction::toggled, this, [this](bool checked) {
        if (!checked && roundRectangleDock_) {
            roundRectangleDock_->hide();
        }
    });
    createToolbar->addSeparator();
    createTextAction_ = addToolAction(createToolbar, QStringLiteral("Text"), QStringLiteral("text.bmp"), true, QStringLiteral("VOC_VECTEXT"));
    createTextAction_->setProperty("implemented", true);
    createTextAction_->setProperty("createOperation", true);
    createTextAction_->setEnabled(true);
    createTextAction_->setStatusTip(QStringLiteral("VOC_VECTEXT"));
    createActionGroup->addAction(createTextAction_);
    connect(createTextAction_, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            showTextDockForCreate(false);
        } else if (textDockMode_ == TextDockMode::CreateText) {
            cancelTextDockOperation();
        }
    });
    createFrameTextAction_ = addToolAction(createToolbar, QStringLiteral("Frame Text"), QStringLiteral("drawtext.bmp"), true, QStringLiteral("VOC_VECFRAMETEXT_EMPTY"));
    createFrameTextAction_->setProperty("implemented", true);
    createFrameTextAction_->setProperty("createOperation", true);
    createFrameTextAction_->setEnabled(true);
    createFrameTextAction_->setStatusTip(QStringLiteral("VOC_VECFRAMETEXT_EMPTY"));
    createActionGroup->addAction(createFrameTextAction_);
    connect(createFrameTextAction_, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            startFrameTextBoxCreate();
        } else if (textDockMode_ == TextDockMode::CreateFrameTextBox) {
            cancelTextDockOperation();
        }
    });
    connect(createActionGroup, &QActionGroup::triggered, this, [this](QAction* action) {
        if (!action->property("createOperation").toBool() && canvas_->interactionMode() == QVedWidget::InteractionMode::CreateLine) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
        }
        if (action != createTextAction_ && action != createFrameTextAction_ &&
            (textDockMode_ == TextDockMode::CreateText || textDockMode_ == TextDockMode::CreateFrameTextBox)) {
            textDockMode_ = TextDockMode::None;
            if (textDock_) {
                textDock_->hide();
            }
        }
    });

    addToolBarBreak(Qt::TopToolBarArea);
    auto* modifyToolbar = new QToolBar(QStringLiteral("Modify"), this);
    modifyToolbar->setObjectName(QStringLiteral("ModifyToolbar"));
    auto* modifyActionGroup = new QActionGroup(modifyToolbar);
    modifyActionGroup->setExclusive(true);
    addToolBar(Qt::TopToolBarArea, modifyToolbar);
    auto* selectAction = addToolAction(modifyToolbar, QStringLiteral("Select"), QStringLiteral("select.bmp"), true, QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"));
    selectAction_ = selectAction;
    selectAction->setProperty("implemented", true);
    selectAction->setEnabled(true);
    selectAction->setStatusTip(QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"));
    modifyActionGroup->addAction(selectAction);
    connect(selectAction, &QAction::triggered, this, [this, selectAction](bool checked) {
        if (checked) {
            deactivateViewTools();
            if (editor_) {
                editor_->SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::SelectObject);
            setActiveOperationStatus(QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"));
            updateMouseToleranceCrossCursor();
            statusBar()->showMessage(QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"), 1500);
        } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::SelectObject) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            setActiveOperationStatus(QString());
        }
    });
    auto* selectScaleAction = addToolAction(modifyToolbar, QStringLiteral("Select/Scale"), QStringLiteral("selectscale0.bmp"), true, QStringLiteral("VOM_SELECTMOVE_OBJECT_SCALE_FRAME"));
    selectScaleAction->setProperty("implemented", true);
    selectScaleAction->setEnabled(true);
    selectScaleAction->setStatusTip(QStringLiteral("VOM_SELECTMOVE_OBJECT_SCALE_FRAME"));
    modifyActionGroup->addAction(selectScaleAction);
    connect(selectScaleAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            deactivateViewTools();
            if (editor_) {
                editor_->SetOperation(VOM_SELECTMOVE_OBJ_SCALE_FRAME);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::SelectObject);
            setActiveOperationStatus(QStringLiteral("VOM_SELECTMOVE_OBJECT_SCALE_FRAME"));
            updateMouseToleranceCrossCursor();
            statusBar()->showMessage(QStringLiteral("VOM_SELECTMOVE_OBJECT_SCALE_FRAME"), 1500);
        } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::SelectObject) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            setActiveOperationStatus(QString());
        }
    });
    auto* deleteShortcutAction = new QAction(this);
    deleteShortcutAction->setShortcut(QKeySequence::Delete);
    addAction(deleteShortcutAction);
    connect(deleteShortcutAction, &QAction::triggered, this, [this, selectAction] {
        if (editor_) {
            editor_->SetOperation(VOM_DELETE_OBJECT);
            if (operationManager_) {
                operationManager_->KeyDown(VIRTKEY_DELETE, VKState_KEY_UNKNOWN);
            }
            editor_->SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
        }
        deactivateViewTools();
        canvas_->setInteractionMode(QVedWidget::InteractionMode::SelectObject);
        selectAction->setChecked(true);
        setActiveOperationStatus(QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"));
        updateGroupCommandActions();
        statusBar()->showMessage(QStringLiteral("VOM_DELETE_OBJECT"), 1500);
    });
    moveObjectAction_ = addToolAction(modifyToolbar, QStringLiteral("Move Object"), QStringLiteral("move_object.bmp"), true, QStringLiteral("VOM_MOVE_OBJECT"));
    moveObjectAction_->setProperty("implemented", true);
    moveObjectAction_->setEnabled(true);
    moveObjectAction_->setStatusTip(QStringLiteral("VOM_MOVE_OBJECT"));
    modifyActionGroup->addAction(moveObjectAction_);
    connect(moveObjectAction_, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            deactivateViewTools();
            if (editor_) {
                editor_->SetOperation(VOM_MOVE_OBJECT);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::MoveObject);
            setActiveOperationStatus(QStringLiteral("VOM_MOVE_OBJECT"));
            statusBar()->showMessage(QStringLiteral("VOM_MOVE_OBJECT"), 1500);
        } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::MoveObject) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            setActiveOperationStatus(QString());
        }
    });
    auto addModifyOperationAction = [&](const QString& text, const QString& iconName, TDViewOperationEnum operation, const QString& operationName) {
        auto* action = addToolAction(modifyToolbar, text, iconName, true, operationName);
        action->setProperty("implemented", true);
        action->setEnabled(true);
        action->setStatusTip(operationName);
        modifyActionGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, operation, operationName](bool checked) {
            if (checked) {
                deactivateViewTools();
                if (editor_) {
                    editor_->SetOperation(operation);
                }
                canvas_->setInteractionMode(QVedWidget::InteractionMode::ModifyObject);
                setActiveOperationStatus(operationName);
                statusBar()->showMessage(operationName, 1500);
            } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::ModifyObject) {
                canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
                setActiveOperationStatus(QString());
            }
        });
        return action;
    };
    moveNodeAction_ = addModifyOperationAction(QStringLiteral("Move Node"), QStringLiteral("move_node.bmp"), VOM_MOVE_NODE, QStringLiteral("VOM_MOVE_NODE"));
    moveBsplineControlPointAction_ = addModifyOperationAction(QStringLiteral("Move B-Spline Node"), QStringLiteral("move_bspline_node.bmp"), VOM_MOVE_BSPLINE_CONTROLPOINT, QStringLiteral("VOM_MOVE_BSPLINE_CONTROLPOINT"));
    insertNodeAction_ = addModifyOperationAction(QStringLiteral("Insert Node"), QStringLiteral("insert_node.bmp"), VOM_INSERT_NODE, QStringLiteral("VOM_INSERT_NODE"));
    deleteNodeAction_ = addModifyOperationAction(QStringLiteral("Delete Node"), QStringLiteral("delete_node.bmp"), VOM_DELETE_NODE, QStringLiteral("VOM_DELETE_NODE"));
    auto* deleteObjectAction = addToolAction(modifyToolbar, QStringLiteral("Delete Object"), QStringLiteral("delete_interactive.bmp"), true, QStringLiteral("VOM_DELETE_OBJECT"));
    deleteObjectAction->setProperty("implemented", true);
    deleteObjectAction->setEnabled(true);
    deleteObjectAction->setStatusTip(QStringLiteral("VOM_DELETE_OBJECT"));
    modifyActionGroup->addAction(deleteObjectAction);
    connect(deleteObjectAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            deactivateViewTools();
            if (editor_) {
                editor_->SetOperation(VOM_DELETE_OBJECT);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::DeleteObject);
            setActiveOperationStatus(QStringLiteral("VOM_DELETE_OBJECT"));
            statusBar()->showMessage(QStringLiteral("VOM_DELETE_OBJECT"), 1500);
        } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::DeleteObject) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            setActiveOperationStatus(QString());
        }
    });
    modifyToolbar->addSeparator();
    auto* changeEdgeRoundNodeAction = addToolAction(
        modifyToolbar,
        QStringLiteral("Change Edge Round Node"),
        QStringLiteral("change_node.bmp"),
        true,
        QStringLiteral("VOM_CHANGE_EDGE_ROUND_NODE"));
    changeEdgeRoundNodeAction_ = changeEdgeRoundNodeAction;
    changeEdgeRoundNodeAction->setProperty("implemented", true);
    changeEdgeRoundNodeAction->setStatusTip(QStringLiteral("VOM_CHANGE_EDGE_ROUND_NODE"));
    changeEdgeRoundNodeAction->setEnabled(false);
    modifyActionGroup->addAction(changeEdgeRoundNodeAction);
    connect(changeEdgeRoundNodeAction, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            deactivateViewTools();
            if (editor_) {
                editor_->SetOperation(VOM_CHANGE_EDGE_ROUND_NODE);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::ModifyObject);
            setActiveOperationStatus(QStringLiteral("VOM_CHANGE_EDGE_ROUND_NODE"));
            statusBar()->showMessage(QStringLiteral("VOM_CHANGE_EDGE_ROUND_NODE"), 1500);
        } else if (canvas_->interactionMode() == QVedWidget::InteractionMode::ModifyObject) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            setActiveOperationStatus(QString());
        }
    });
    rotateObjectAction_ = addModifyOperationAction(
        QStringLiteral("Rotate Object"),
        QStringLiteral("rotate_object.bmp"),
        VOM_ROTATE_OBJECT_ACTIVEPOINT,
        QStringLiteral("VOM_ROTATE_OBJECT_ACTIVEPOINT"));
    connect(rotateObjectAction_, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            showRotateDock();
        }
    });
    connect(rotateObjectAction_, &QAction::toggled, this, [this](bool checked) {
        if (!checked && rotateDock_) {
            rotateDock_->hide();
        }
    });
    scaleObjectAction_ = addModifyOperationAction(QStringLiteral("Scale Object"), QStringLiteral("scale_object.bmp"), VOM_SCALE_OBJECT_3POINT, QStringLiteral("VOM_SCALE_OBJECT_3POINT"));
    modifyCurveAttributeAction_ = addModifyOperationAction(
        QStringLiteral("Modify Curve Attribute"),
        QStringLiteral("change_bspline_attr.bmp"),
        VOM_MODIFY_CURVE_ATTRIBUTE,
        QStringLiteral("VOM_MODIFY_CURVE_ATTRIBUTE"));
    modifyCurveAttributeAction_->setEnabled(false);
    connect(modifyCurveAttributeAction_, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            showPolyCurveDock();
        }
    });
    connect(modifyCurveAttributeAction_, &QAction::toggled, this, [this](bool checked) {
        if (!checked && curveDock_ &&
            operationManager_ &&
            operationManager_->GetAktiveOperationType() == VOM_MODIFY_CURVE_ATTRIBUTE) {
            curveDock_->hide();
        }
    });
    addToolAction(modifyToolbar, QStringLiteral("Object Position/Size"), QStringLiteral("object_position_size.bmp"));
    modifyTextAction_ = addToolAction(modifyToolbar, QStringLiteral("Text Properties"), QStringLiteral("text_properties.bmp"), true, QStringLiteral("VOM_VECTEXT"));
    modifyTextAction_->setProperty("implemented", true);
    modifyTextAction_->setStatusTip(QStringLiteral("VOM_VECTEXT"));
    modifyTextAction_->setEnabled(false);
    modifyActionGroup->addAction(modifyTextAction_);
    connect(modifyTextAction_, &QAction::triggered, this, [this](bool checked) {
        if (checked) {
            showTextDockForModify();
        } else if (textDockMode_ == TextDockMode::Modify) {
            cancelTextDockOperation();
        }
    });
    modifyToolbar->addSeparator();
    auto* makeGroupAction = addToolAction(modifyToolbar, QStringLiteral("Make Group"), QStringLiteral("make_group.bmp"), false, QStringLiteral("cmdMakeGroup"));
    makeGroupAction_ = makeGroupAction;
    makeGroupAction->setProperty("implemented", true);
    makeGroupAction->setStatusTip(QStringLiteral("cmdMakeGroup"));
    connect(makeGroupAction, &QAction::triggered, this, [this] {
        if (operationManager_ && operationManager_->MakeGroup()) {
            statusBar()->showMessage(QStringLiteral("cmdMakeGroup"), 1500);
            updateGroupCommandActions();
            updateModifyCommandActions();
        }
    });
    auto* resolveGroupAction = addToolAction(modifyToolbar, QStringLiteral("Resolve Group"), QStringLiteral("resolve_group.bmp"), false, QStringLiteral("cmdResolveGroup"));
    resolveGroupAction_ = resolveGroupAction;
    resolveGroupAction->setProperty("implemented", true);
    resolveGroupAction->setStatusTip(QStringLiteral("cmdResolveGroup"));
    connect(resolveGroupAction, &QAction::triggered, this, [this] {
        if (operationManager_ && operationManager_->ResolveAllSelectedGroups()) {
            statusBar()->showMessage(QStringLiteral("cmdResolveGroup"), 1500);
            updateGroupCommandActions();
            updateModifyCommandActions();
        }
    });
    lockObjectAction_ = addToolAction(modifyToolbar, QStringLiteral("Lock Object"), QStringLiteral("forbidden_sign.bmp"), true, QStringLiteral("cmdLockObject"));
    lockObjectAction_->setProperty("implemented", true);
    lockObjectAction_->setEnabled(false);
    lockObjectAction_->setStatusTip(QStringLiteral("cmdLockObject"));
    connect(lockObjectAction_, &QAction::triggered, this, [this] {
        if (!model_) {
            return;
        }

        const bool resizeLocked = model_->GetLockResizeForSelectedObjects() != 0;
        const bool positionLocked = model_->GetLockPositionForSelectedObjects() != 0;
        const bool resizeChanged = model_->SetLockResizeForSelectedObjects(!resizeLocked);
        const bool positionChanged = model_->SetLockPositionForSelectedObjects(!positionLocked);
        if (resizeChanged || positionChanged) {
            canvas_->update();
            updateModifyCommandActions();
            statusBar()->showMessage(QStringLiteral("cmdLockObject"), 1500);
        } else {
            QApplication::beep();
            updateModifyCommandActions();
        }
    });
    addToolAction(modifyToolbar, QStringLiteral("Delete Permanently"), QStringLiteral("delete.bmp"), true);
    connect(modifyActionGroup, &QActionGroup::triggered, this, [this, selectAction, deleteObjectAction](QAction* action) {
        if (action != selectAction && action != moveObjectAction_ && action != deleteObjectAction) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
        }
    });

    addToolBarBreak(Qt::TopToolBarArea);
    auto* viewToolbar = new QToolBar(QStringLiteral("View"), this);
    viewToolbar->setObjectName(QStringLiteral("ViewToolbar"));
    addToolBar(Qt::TopToolBarArea, viewToolbar);
    zoomAction_ = addToolAction(viewToolbar, QStringLiteral("Zoom"), QStringLiteral("zoom.bmp"), true, QStringLiteral("VOP_ZOOM_INOUT"));
    zoomAction_->setProperty("implemented", true);
    zoomAction_->setEnabled(true);
    zoomAction_->setStatusTip(QStringLiteral("VOP_ZOOM_INOUT"));
    connect(zoomAction_, &QAction::triggered, this, [this] {
        if (zoomAction_->isChecked()) {
            if (panAction_) {
                panAction_->setChecked(false);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            canvas_->setPanToolEnabled(false);
        }
        canvas_->setZoomToolEnabled(zoomAction_->isChecked());
        statusBar()->showMessage(QStringLiteral("VOP_ZOOM_INOUT %1").arg(zoomAction_->isChecked() ? QStringLiteral("on") : QStringLiteral("off")), 1500);
    });
    panAction_ = viewToolbar->addAction(QIcon(QStringLiteral(":/ved/toolbar/drag_document.png")), QStringLiteral("Pan"));
    panAction_->setCheckable(true);
    panAction_->setToolTip(QStringLiteral("Drag document"));
    panAction_->setStatusTip(QStringLiteral("Drag document"));
    panAction_->setProperty("implemented", true);
    panAction_->setEnabled(true);
    connect(panAction_, &QAction::triggered, this, [this] {
        if (panAction_->isChecked()) {
            if (zoomAction_) {
                zoomAction_->setChecked(false);
            }
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
            canvas_->setZoomToolEnabled(false);
        }
        canvas_->setPanToolEnabled(panAction_->isChecked());
        statusBar()->showMessage(
            QStringLiteral("Drag document %1").arg(panAction_->isChecked() ? QStringLiteral("on") : QStringLiteral("off")),
            1500);
    });
    auto* resetViewToolbarAction = viewToolbar->addAction(
        QIcon(QStringLiteral(":/ved/toolbar/reset_view.png")),
        QStringLiteral("Reset View"));
    resetViewToolbarAction->setToolTip(QStringLiteral("Reset View"));
    resetViewToolbarAction->setStatusTip(QStringLiteral("Reset View"));
    resetViewToolbarAction->setProperty("implemented", true);
    connect(resetViewToolbarAction, &QAction::triggered, this, [this] {
        canvas_->resetView();
        statusBar()->showMessage(QStringLiteral("View reset"), 2500);
    });
    viewToolbar->addSeparator();
    showGridAction_ = addToolAction(viewToolbar, QStringLiteral("Show Grid"), QStringLiteral("show_grid.bmp"), true, QStringLiteral("cmdShowGrid"));
    showGridAction_->setProperty("implemented", true);
    showGridAction_->setEnabled(true);
    showGridAction_->setStatusTip(QStringLiteral("cmdShowGrid"));
    connect(showGridAction_, &QAction::triggered, this, [this] {
        canvas_->setShowGrid(showGridAction_->isChecked());
        statusBar()->showMessage(QStringLiteral("cmdShowGrid %1").arg(showGridAction_->isChecked() ? QStringLiteral("on") : QStringLiteral("off")), 1500);
    });
    gridLockAction_ = addToolAction(viewToolbar, QStringLiteral("Use Grid"), QStringLiteral("use_grid.bmp"), true, QStringLiteral("cmdLockGrid"));
    gridLockAction_->setProperty("implemented", true);
    gridLockAction_->setEnabled(true);
    gridLockAction_->setStatusTip(QStringLiteral("cmdLockGrid"));
    connect(gridLockAction_, &QAction::triggered, this, [this] {
        canvas_->setGridLock(gridLockAction_->isChecked());
        statusBar()->showMessage(QStringLiteral("cmdLockGrid %1").arg(gridLockAction_->isChecked() ? QStringLiteral("on") : QStringLiteral("off")), 1500);
    });
    showRulersAction_ = viewToolbar->addAction(makeRulersIcon(), QStringLiteral("Show Rulers"));
    showRulersAction_->setCheckable(true);
    showRulersAction_->setToolTip(QStringLiteral("cmdShowRulers"));
    showRulersAction_->setStatusTip(QStringLiteral("cmdShowRulers"));
    showRulersAction_->setProperty("implemented", true);
    connect(showRulersAction_, &QAction::triggered, this, [this] {
        canvas_->setShowRulers(showRulersAction_->isChecked());
        statusBar()->showMessage(
            QStringLiteral("cmdShowRulers %1")
                .arg(showRulersAction_->isChecked() ? QStringLiteral("on") : QStringLiteral("off")),
            1500);
    });
    mouseToleranceCrossAction_ = viewToolbar->addAction(makeMouseToleranceCrossIcon(), QStringLiteral("Mouse Tolerance Cross"));
    mouseToleranceCrossAction_->setCheckable(true);
    mouseToleranceCrossAction_->setToolTip(QStringLiteral("cmdShowMouseToleranceCross"));
    mouseToleranceCrossAction_->setStatusTip(QStringLiteral("cmdShowMouseToleranceCross"));
    mouseToleranceCrossAction_->setProperty("implemented", true);
    connect(mouseToleranceCrossAction_, &QAction::triggered, this, [this] {
        canvas_->setShowMouseToleranceCross(mouseToleranceCrossAction_->isChecked());
        updateMouseToleranceCrossCursor();
        statusBar()->showMessage(
            QStringLiteral("cmdShowMouseToleranceCross %1")
                .arg(mouseToleranceCrossAction_->isChecked() ? QStringLiteral("on") : QStringLiteral("off")),
            1500);
    });
    viewToolbar->addSeparator();
    auto addAlignAction = [this, viewToolbar](const QString& text, const QString& iconName, TDVecAlignMode mode, const QString& commandName) {
        auto* action = addToolAction(viewToolbar, text, iconName, false, commandName);
        action->setProperty("implemented", true);
        action->setEnabled(false);
        action->setStatusTip(commandName);
        alignActions_.push_back(action);
        connect(action, &QAction::triggered, this, [this, mode, commandName] {
            if (operationManager_ && operationManager_->AlignSelectedObjects(mode)) {
                statusBar()->showMessage(commandName, 1500);
                updateGroupCommandActions();
                updateModifyCommandActions();
            }
        });
        return action;
    };
    addAlignAction(QStringLiteral("Align Left"), QStringLiteral("align_left.bmp"), TDVecAlignMode::Left, QStringLiteral("cmdAlignLeft"));
    addAlignAction(QStringLiteral("Align Center"), QStringLiteral("align_center.bmp"), TDVecAlignMode::HorizontalCenter, QStringLiteral("cmdAlignCenter"));
    addAlignAction(QStringLiteral("Align Right"), QStringLiteral("align_right.bmp"), TDVecAlignMode::Right, QStringLiteral("cmdAlignRight"));
    addAlignAction(QStringLiteral("Align Top"), QStringLiteral("align_top.bmp"), TDVecAlignMode::Top, QStringLiteral("cmdAlignTop"));
    addAlignAction(QStringLiteral("Align Middle"), QStringLiteral("align_middle.bmp"), TDVecAlignMode::VerticalMiddle, QStringLiteral("cmdAlignMiddle"));
    addAlignAction(QStringLiteral("Align Bottom"), QStringLiteral("align_bottom.bmp"), TDVecAlignMode::Bottom, QStringLiteral("cmdAlignBottom"));
    updateGroupCommandActions();
    updateModifyCommandActions();
}

QAction* MainWindow::addToolAction(
    QToolBar* toolbar,
    const QString& text,
    const QString& iconName,
    bool checkable,
    const QString& toolTip) {
    auto* action = iconName.isEmpty()
        ? toolbar->addAction(text)
        : toolbar->addAction(QIcon(QStringLiteral(":/ved/toolbar/%1").arg(iconName)), text);
    action->setCheckable(checkable);
    action->setToolTip(toolTip);
    action->setStatusTip(QStringLiteral("%1 - not implemented yet").arg(toolTip.isEmpty() ? text : toolTip));
    action->setEnabled(false);
    connect(action, &QAction::triggered, this, [this, text](bool) {
        auto* action = qobject_cast<QAction*>(sender());
        if (action && action->property("implemented").toBool()) {
            return;
        }
        statusBar()->showMessage(QStringLiteral("%1 is not implemented yet").arg(text), 2500);
    });
    return action;
}
