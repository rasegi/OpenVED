#include "MainWindow.h"

#include "DocumentSetupDialog.h"
#include "QVedWidget.h"
#include "vec_document_settings.h"
#include "vec_edit_cad.h"
#include "vec_font.h"
#include "vec_font_manager.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vec_text.h"
#include "ved_model_io.h"
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
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleValidator>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QStringList>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolButton>
#include <QToolBar>
#include <QVBoxLayout>
#include <Qt>

#include <cstdio>
#include <cstddef>
#include <span>

namespace {
constexpr int kMainWindowStateVersion = 1;
constexpr auto kSettingsGeometry = "mainWindow/geometry";
constexpr auto kSettingsState = "mainWindow/state";
constexpr auto kSettingsLastDocumentDirectory = "documents/lastDirectory";
constexpr auto kSettingsLastDocumentPath = "documents/lastDocumentPath";
constexpr auto kSettingsConvertSystemFonts = "fonts/convertSystemFonts";

bool supportsInsertDeleteNode(const TDVecObject* object) {
    if (!object) {
        return false;
    }

    switch (object->GetType()) {
    case VECOBJ_POL:
    case VECOBJ_POLAREA:
    case VECOBJ_BSPLINE:
    case VECOBJ_POLYCURVE:
    case VECOBJ_POLYCURVEAREA:
        return true;
    default:
        return false;
    }
}

QString vedFileFilter() {
    return QStringLiteral("VED documents (*.ved);;All files (*)");
}

QString dialogStartPath(const QString& currentDocumentPath, const QString& lastDocumentDirectory, const QString& fallbackFileName) {
    if (!currentDocumentPath.isEmpty()) {
        return currentDocumentPath;
    }
    if (!lastDocumentDirectory.isEmpty()) {
        return QDir(lastDocumentDirectory).filePath(fallbackFileName);
    }
    return fallbackFileName;
}

QString fontWarningText(const VEDFontResolutionResult& fontResolution) {
    QStringList lines;
    for (const VEDFontResolutionWarning& warning : fontResolution.warnings) {
        const QString requested = QString::fromStdString(warning.requestedFontId);
        const QString resolved = QString::fromStdString(warning.resolvedFontId);
        switch (warning.kind) {
        case VEDFontFallbackKind::RequestedMissingUsedArialUnicodeMS:
            lines.push_back(QStringLiteral("Font '%1' was not found. The document was opened using '%2'.").arg(requested, resolved));
            break;
        case VEDFontFallbackKind::RequestedMissingUsedSegoeUI:
            lines.push_back(QStringLiteral("Font '%1' was not found. The document was opened using '%2'.").arg(requested, resolved));
            break;
        case VEDFontFallbackKind::RequestedMissingUsedWpsDefault:
            lines.push_back(QStringLiteral("Font '%1' was not found. The document was opened using '%2'.").arg(requested, resolved));
            break;
        case VEDFontFallbackKind::NoFallbackAvailable:
            lines.push_back(QStringLiteral("Font '%1' was not found and no fallback font is available.").arg(requested));
            break;
        case VEDFontFallbackKind::None:
            break;
        }
    }
    return lines.join(QLatin1Char('\n'));
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      canvas_(new QVedWidget(this)),
      selectAction_(nullptr),
      cutAction_(nullptr),
      copyAction_(nullptr),
      pasteAction_(nullptr),
      undoAction_(nullptr),
      redoAction_(nullptr),
      makeGroupAction_(nullptr),
      resolveGroupAction_(nullptr),
      lockObjectAction_(nullptr),
      moveObjectAction_(nullptr),
      moveNodeAction_(nullptr),
      insertNodeAction_(nullptr),
      deleteNodeAction_(nullptr),
      moveBsplineControlPointAction_(nullptr),
      rotateObjectAction_(nullptr),
      scaleObjectAction_(nullptr),
      changeEdgeRoundNodeAction_(nullptr),
      modifyCurveAttributeAction_(nullptr),
      createTextAction_(nullptr),
      createFrameTextAction_(nullptr),
      modifyTextAction_(nullptr),
      zoomAction_(nullptr),
      panAction_(nullptr),
      showGridAction_(nullptr),
      gridLockAction_(nullptr),
      showRulersAction_(nullptr),
      mouseToleranceCrossAction_(nullptr),
      statusBarSplitter_(nullptr),
      activeOperationLabel_(nullptr),
      pageFormatLabel_(nullptr),
      coordinateLabel_(nullptr),
      curveDock_(nullptr),
      curveShowPolygonCheck_(nullptr),
      curveShowControlsCheck_(nullptr),
      curveCopyCheck_(nullptr),
      curveDegreeLabel_(nullptr),
      curveDegreeEdit_(nullptr),
      curveResolutionEdit_(nullptr),
      roundRectangleDock_(nullptr),
      roundRectangleRadiusEdit_(nullptr),
      roundRectangleResolutionEdit_(nullptr),
      roundRectangleShowPolygonCheck_(nullptr),
      roundRectangleShowControlsCheck_(nullptr),
      roundRectangleWidthLabel_(nullptr),
      roundRectangleHeightLabel_(nullptr),
      rotateDock_(nullptr),
      rotateAngleEdit_(nullptr),
      rotateCopyCheck_(nullptr),
      rotateSelectCopyCheck_(nullptr),
      rotateObjectCountLabel_(nullptr),
      rotatePivotLabel_(nullptr),
      textDock_(nullptr),
      textEdit_(nullptr),
      textXScaleSpin_(nullptr),
      textYScaleSpin_(nullptr),
      textAngleSpin_(nullptr),
      textInclineSpin_(nullptr),
      textLineSpacingSpin_(nullptr),
      textCharSpacingSpin_(nullptr),
      textResolutionSpin_(nullptr),
      textFontCombo_(nullptr),
      textJustificationCombo_(nullptr),
      textVerticalAlignCombo_(nullptr),
      textVerticalCheck_(nullptr),
      textUnderlineCheck_(nullptr),
      textScaleDependencyCheck_(nullptr),
      textDockMode_(TextDockMode::None) {
    resize(1100, 750);
    setStyleSheet(QStringLiteral(
        "QToolButton:checked {"
        "  background: palette(highlight);"
        "  color: palette(highlighted-text);"
        "  border: 1px solid palette(dark);"
        "  border-radius: 3px;"
        "}"
        "QToolButton:checked:hover {"
        "  background: palette(highlight);"
        "}"));

    setCentralWidget(canvas_);
    initializeEditor();
    updateWindowTitle();

    createMenus();
    createToolBars();
    createCurveDock();
    createRoundRectangleDock();
    createRotateDock();
    createTextDock();
    defaultWindowState_ = saveState(kMainWindowStateVersion);
    readSettings();
    updateModifyCommandActions();
    statusBarSplitter_ = new QSplitter(Qt::Horizontal, this);
    statusBarSplitter_->setChildrenCollapsible(false);
    statusBarSplitter_->setHandleWidth(6);
    statusBarSplitter_->setStyleSheet(QStringLiteral(
        "QSplitter::handle { background: palette(mid); border-radius: 2px; }"));
    activeOperationLabel_ = new QLabel(QStringLiteral("Operation: -"), this);
    activeOperationLabel_->setMinimumWidth(150);
    activeOperationLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusBarSplitter_->addWidget(activeOperationLabel_);
    pageFormatLabel_ = new QLabel(this);
    pageFormatLabel_->setMinimumWidth(150);
    pageFormatLabel_->setAlignment(Qt::AlignCenter);
    statusBarSplitter_->addWidget(pageFormatLabel_);
    updatePageFormatStatus();
    coordinateLabel_ = new QLabel(QStringLiteral("X: -  Y: -"), this);
    coordinateLabel_->setMinimumWidth(120);
    coordinateLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusBarSplitter_->addWidget(coordinateLabel_);
    statusBarSplitter_->setStretchFactor(0, 3);
    statusBarSplitter_->setStretchFactor(1, 3);
    statusBarSplitter_->setStretchFactor(2, 2);
    statusBar()->addPermanentWidget(statusBarSplitter_, 1);
    statusBar()->showMessage(QStringLiteral("Ready"));

    const QString lastDoc = QSettings().value(QString::fromLatin1(kSettingsLastDocumentPath)).toString();
    if (!lastDoc.isEmpty() && QFile::exists(lastDoc) && loadDefaultVecFont() && fontManager_) {
        QFile file(lastDoc);
        if (file.open(QIODevice::ReadOnly)) {
            const QByteArray data = file.readAll();
            VEDModelReadResult result = LoadVecModelFromBytes(data.constData(), static_cast<std::size_t>(data.size()), *fontManager_);
            if (result.Ok()) {
                installModel(std::move(result.model), &result.viewState);
                currentDocumentPath_ = lastDoc;
                lastDocumentDirectory_ = QFileInfo(lastDoc).absolutePath();
                updateWindowTitle();
                showFontResolutionWarnings(result.fontResolution);
            }
        }
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::initializeEditor() {
    model_ = std::make_unique<TDVecModel>();
    const auto& initPage = model_->PageSettings();
    model_->SetTopLeftArea({initPage.pageOriginX, initPage.pageOriginY});
    model_->SetBottomRightArea({initPage.pageOriginX + initPage.widthReal, initPage.pageOriginY + initPage.heightReal});
    canvas_->setDocumentSettings(&model_->DocumentSettings());

    editor_ = std::make_unique<TDVecEditCad>();
    editor_->SetVecModel(model_.get());
    editor_->RegisterViewInterface(canvas_->viewInterface());
    operationManager_ = std::make_unique<TDViewOperationManager>(model_.get(), editor_.get());
    loadDefaultVecFont();

    canvas_->setPaintContentCallback([this](TDVecViewInterfaceBase* view) {
        if (editor_) {
            editor_->PaintContentOnView(view);
        }
    });
    canvas_->setOperationMouseDownCallback([this](TDOPVirtMouseButton button, TDOPVirtKeyState shift, TDMatPoint point) {
        if (button == VIRTMOUSEBTM_RIGHT && cancelDialogOperationFromRightClick()) {
            return;
        }
        if (editor_) {
            editor_->MouseDown(canvas_->viewInterface(), button, shift, point.x, point.y);
            syncUiWithActiveOperation();
            updateGroupCommandActions();
            updateModifyCommandActions();
            updateHistoryCommandActions();
            updateMouseToleranceCrossCursor();
        }
    });
    canvas_->setOperationMouseUpCallback([this](TDOPVirtMouseButton button, TDOPVirtKeyState shift, TDMatPoint point) {
        if (editor_) {
            editor_->MouseUp(canvas_->viewInterface(), button, shift, point.x, point.y);
            syncUiWithActiveOperation();
            updateGroupCommandActions();
            updateModifyCommandActions();
            updateHistoryCommandActions();
            updateMouseToleranceCrossCursor();
        }
    });
    canvas_->setOperationMouseMoveCallback([this](TDOPVirtMouseButton button, TDOPVirtKeyState shift, TDMatPoint point) {
        if (editor_) {
            editor_->MouseMove(canvas_->viewInterface(), button, shift, point.x, point.y);
            refreshRoundRectangleDockFromExtVar();
            refreshRotateDockFromExtVar();
            updateMouseToleranceCrossCursor();
        }
    });
    canvas_->setMouseCoordinateCallback([this](TDMatPoint point, bool valid) {
        updateCoordinateStatus(point, valid);
    });
}

void MainWindow::installModel(std::unique_ptr<TDVecModel> model, const VEDDocumentViewState* viewState) {
    if (!model) {
        return;
    }

    if (textDockMode_ != TextDockMode::None) {
        cancelTextDockOperation();
    }
    if (curveDock_) {
        curveDock_->hide();
    }
    if (roundRectangleDock_) {
        roundRectangleDock_->hide();
    }
    if (rotateDock_) {
        rotateDock_->hide();
    }

    model_ = std::move(model);
    canvas_->setDocumentSettings(&model_->DocumentSettings());
    if (editor_) {
        editor_->TmpClear();
        editor_->SetVecModel(model_.get());
    }
    operationManager_ = std::make_unique<TDViewOperationManager>(model_.get(), editor_.get());
    if (editor_) {
        editor_->SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
    }
    if (selectAction_) {
        selectAction_->setChecked(true);
    }
    canvas_->setInteractionMode(QVedWidget::InteractionMode::SelectObject);
    setActiveOperationStatus(QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"));
    updateGroupCommandActions();
    updateModifyCommandActions();
    updateHistoryCommandActions();
    updateMouseToleranceCrossCursor();
    updatePageFormatStatus();
    if (viewState && viewState->present) {
        canvas_->applyDocumentViewState(*viewState);
    } else {
        canvas_->resetView();
    }
    syncViewActionsFromCanvas();
}

void MainWindow::syncViewActionsFromCanvas() {
    if (showGridAction_) {
        showGridAction_->setChecked(canvas_->showGrid());
    }
    if (gridLockAction_) {
        gridLockAction_->setChecked(canvas_->gridLock());
    }
    if (showRulersAction_) {
        showRulersAction_->setChecked(canvas_->showRulers());
    }
}

void MainWindow::showFontResolutionWarnings(const VEDFontResolutionResult& fontResolution) {
    const QString message = fontWarningText(fontResolution);
    if (message.isEmpty()) {
        return;
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Font fallback warning"),
        message);
}

void MainWindow::newDocument() {
    if (!confirmDiscardUnsavedChanges()) {
        return;
    }

    const TDVecDocumentSettings currentDefaults = model_
        ? model_->DocumentSettings() : TDVecDocumentSettings{};
    DocumentSetupDialog dialog(currentDefaults, this);
    dialog.setWindowTitle(QStringLiteral("New Document"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const TDVecDocumentSettings settings = dialog.documentSettings();
    auto model = std::make_unique<TDVecModel>();
    model->SetDocumentSettings(settings);
    const auto& ps = settings.pageSettings;
    model->SetTopLeftArea({ps.pageOriginX, ps.pageOriginY});
    model->SetBottomRightArea({ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal});

    installModel(std::move(model));
    model_->SetChanged(false);
    currentDocumentPath_.clear();
    updateWindowTitle();
    statusBar()->showMessage(QStringLiteral("New document"), 2500);
}

void MainWindow::openDocument() {
    if (!confirmDiscardUnsavedChanges()) {
        return;
    }

    if (!loadDefaultVecFont() || !fontManager_) {
        QMessageBox::warning(
            this,
            QStringLiteral("Open VED document"),
            QStringLiteral("Default font could not be loaded."));
        return;
    }

    const QString fileName = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open VED document"),
        dialogStartPath(currentDocumentPath_, lastDocumentDirectory_, QString()),
        vedFileFilter());
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Open VED document"),
            QStringLiteral("Could not open '%1'.").arg(fileName));
        return;
    }

    const QByteArray data = file.readAll();
    VEDModelReadResult result = LoadVecModelFromBytes(data.constData(), static_cast<std::size_t>(data.size()), *fontManager_);
    if (!result.Ok()) {
        QMessageBox::warning(
            this,
            QStringLiteral("Open VED document"),
            QStringLiteral("Could not read '%1'.").arg(fileName));
        return;
    }

    installModel(std::move(result.model), &result.viewState);
    currentDocumentPath_ = fileName;
    lastDocumentDirectory_ = QFileInfo(fileName).absolutePath();
    updateWindowTitle();
    writeSettings();
    statusBar()->showMessage(QStringLiteral("Opened %1").arg(QFileInfo(fileName).fileName()), 2500);
    showFontResolutionWarnings(result.fontResolution);
}

bool MainWindow::saveDocument() {
    if (!model_) {
        return true;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save VED document"),
        dialogStartPath(currentDocumentPath_, lastDocumentDirectory_, QStringLiteral("untitled.ved")),
        vedFileFilter());
    if (fileName.isEmpty()) {
        return false;
    }
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName += QStringLiteral(".ved");
    }

    const VEDModelWriteResult result = SaveVecModelToBytes(*model_, canvas_->documentViewState());
    if (!result.Ok()) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save VED document"),
            QStringLiteral("Could not serialize the current document."));
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save VED document"),
            QStringLiteral("Could not write '%1'.").arg(fileName));
        return false;
    }

    const auto* bytes = reinterpret_cast<const char*>(result.bytes.data());
    const qint64 written = file.write(bytes, static_cast<qint64>(result.bytes.size()));
    if (written != static_cast<qint64>(result.bytes.size())) {
        QMessageBox::warning(
            this,
            QStringLiteral("Save VED document"),
            QStringLiteral("Could not write all data to '%1'.").arg(fileName));
        return false;
    }

    currentDocumentPath_ = fileName;
    lastDocumentDirectory_ = QFileInfo(fileName).absolutePath();
    model_->SetChanged(false);
    updateWindowTitle();
    writeSettings();
    statusBar()->showMessage(QStringLiteral("Saved %1").arg(QFileInfo(fileName).fileName()), 2500);
    return true;
}

bool MainWindow::confirmDiscardUnsavedChanges() {
    if (!model_ || !model_->IsChanged()) {
        return true;
    }

    const QMessageBox::StandardButton result = QMessageBox::warning(
        this,
        QStringLiteral("Unsaved changes"),
        QStringLiteral("The current document has unsaved changes."),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (result == QMessageBox::Save) {
        return saveDocument() && model_ && !model_->IsChanged();
    }
    if (result == QMessageBox::Discard) {
        return true;
    }
    return false;
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));

    auto* newAction = fileMenu->addAction(QStringLiteral("&New"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, [this] {
        newDocument();
    });

    auto* openAction = fileMenu->addAction(QStringLiteral("&Open..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this] {
        openDocument();
    });

    auto* saveAction = fileMenu->addAction(QStringLiteral("&Save..."));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] {
        saveDocument();
    });

    fileMenu->addSeparator();

    auto* exportPdfAction = fileMenu->addAction(QStringLiteral("Export &PDF..."));
    connect(exportPdfAction, &QAction::triggered, this, &MainWindow::exportPdf);

    auto* printAction = fileMenu->addAction(QStringLiteral("&Print..."));
    printAction->setShortcut(QKeySequence::Print);
    connect(printAction, &QAction::triggered, this, &MainWindow::printDocument);

    fileMenu->addSeparator();

    auto* editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    undoAction_ = editMenu->addAction(QStringLiteral("&Undo"));
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setEnabled(false);
    connect(undoAction_, &QAction::triggered, this, [this] {
        if (operationManager_ && operationManager_->Undo()) {
            updateGroupCommandActions();
            updateModifyCommandActions();
            statusBar()->showMessage(QStringLiteral("Undo"), 1500);
        }
    });
    redoAction_ = editMenu->addAction(QStringLiteral("&Redo"));
    redoAction_->setShortcut(QKeySequence::Redo);
    redoAction_->setEnabled(false);
    connect(redoAction_, &QAction::triggered, this, [this] {
        if (operationManager_ && operationManager_->Redo()) {
            updateGroupCommandActions();
            updateModifyCommandActions();
            statusBar()->showMessage(QStringLiteral("Redo"), 1500);
        }
    });
    editMenu->addSeparator();

    auto* exitAction = fileMenu->addAction(QStringLiteral("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    auto* formatMenu = menuBar()->addMenu(QStringLiteral("F&ormat"));
    auto* docSetupAction = formatMenu->addAction(QStringLiteral("&Document Setup..."));
    connect(docSetupAction, &QAction::triggered, this, &MainWindow::onDocumentSetup);

    formatMenu->addSeparator();
    convertSystemFontsAction_ = formatMenu->addAction(QStringLiteral("Convert &System Fonts"));
    convertSystemFontsAction_->setCheckable(true);
    convertSystemFontsAction_->setStatusTip(QStringLiteral(
        "Scan and convert installed TrueType/OpenType system fonts for use in text objects"));
#if defined(Q_OS_WASM)
    // No installed system fonts are reachable in the browser build.
    convertSystemFontsAction_->setChecked(false);
    convertSystemFontsAction_->setEnabled(false);
    convertSystemFontsAction_->setToolTip(QStringLiteral("Not available in the browser build"));
#else
    convertSystemFontsAction_->setChecked(
        QSettings().value(QString::fromLatin1(kSettingsConvertSystemFonts), false).toBool());
#endif
    // Connect after the initial setChecked so startup does not emit toggled().
    connect(convertSystemFontsAction_, &QAction::toggled, this, &MainWindow::onToggleSystemFonts);

    auto* viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    auto* resetViewAction = viewMenu->addAction(QStringLiteral("&Reset View"));
    connect(resetViewAction, &QAction::triggered, this, [this] {
        canvas_->resetView();
        statusBar()->showMessage(QStringLiteral("View reset"), 2500);
    });

    auto* helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    auto* aboutAction = helpMenu->addAction(QStringLiteral("&About"));
    connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::about(
            this,
            QStringLiteral("About VED Qt Port"),
            QStringLiteral("Minimal Qt shell for the modern VED port."));
    });
}

bool MainWindow::cancelDialogOperationFromRightClick() {
    if (operationManager_) {
        const TDViewOperationEnum activeOperation = operationManager_->GetAktiveOperationType();
        if (activeOperation == VOC_VECTEXT ||
            activeOperation == VOC_VECFRAMETEXT_EMPTY ||
            activeOperation == VOC_POLYCURVE ||
            activeOperation == VOC_BSPLINE_CONTROLPOINT) {
            return false;
        }
    }

    if (textDockMode_ != TextDockMode::None) {
        cancelTextDockOperation();
        statusBar()->showMessage(QStringLiteral("Operation canceled"), 1500);
        return true;
    }

    if (!operationManager_) {
        return false;
    }

    const TDViewOperationEnum activeOperation = operationManager_->GetAktiveOperationType();
    if (curveDock_ && curveDock_->isVisible() &&
        activeOperation == VOM_MODIFY_CURVE_ATTRIBUTE) {
        cancelCurveDockOperation();
        return true;
    }

    if (roundRectangleDock_ && roundRectangleDock_->isVisible() &&
        activeOperation == VOC_ROUND_RECTANGLE_NOTROTATED) {
        cancelRoundRectangleDockOperation();
        return true;
    }

    if (rotateDock_ && rotateDock_->isVisible() &&
        activeOperation == VOM_ROTATE_OBJECT_ACTIVEPOINT) {
        cancelRotateDockOperation();
        return true;
    }

    return false;
}

void MainWindow::readSettings() {
    QSettings settings;

    const auto geometry = settings.value(QString::fromLatin1(kSettingsGeometry)).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    const auto state = settings.value(QString::fromLatin1(kSettingsState)).toByteArray();
    if (!state.isEmpty()) {
        restoreState(state, kMainWindowStateVersion);
    }
    lastDocumentDirectory_ = settings.value(QString::fromLatin1(kSettingsLastDocumentDirectory)).toString();
    hideCurveDockIfInactive();
    hideRoundRectangleDockIfInactive();
    hideTextDockIfInactive();
}

void MainWindow::writeSettings() const {
    QSettings settings;
    settings.setValue(QString::fromLatin1(kSettingsGeometry), saveGeometry());
    settings.setValue(QString::fromLatin1(kSettingsState), saveState(kMainWindowStateVersion));
    if (!lastDocumentDirectory_.isEmpty()) {
        settings.setValue(QString::fromLatin1(kSettingsLastDocumentDirectory), lastDocumentDirectory_);
    }
    settings.setValue(QString::fromLatin1(kSettingsLastDocumentPath), currentDocumentPath_);
}

void MainWindow::onToggleSystemFonts(bool enabled) {
#if !defined(Q_OS_WASM)
    QSettings().setValue(QString::fromLatin1(kSettingsConvertSystemFonts), enabled);
#endif
    // Rebuild providers only if the font stack has already been initialised;
    // otherwise the next loadDefaultVecFont() picks up the new state lazily.
    if (fontManager_) {
        rebuildFontProviders();
    }
    statusBar()->showMessage(
        enabled ? QStringLiteral("System fonts enabled — installed fonts converted")
                : QStringLiteral("System fonts disabled — only bundled fonts shown"),
        2500);
}

void MainWindow::resetWindowLayout() {
    QSettings settings;
    settings.remove(QString::fromLatin1(kSettingsState));

    restoreState(defaultWindowState_, kMainWindowStateVersion);
    statusBar()->showMessage(QStringLiteral("Window layout reset"), 2500);
}

void MainWindow::syncUiWithActiveOperation() {
    if (!operationManager_ || operationManager_->GetAktiveOperationType() != VOM_SELECT_MOVE_NODE_OBJECT) {
        return;
    }

    if (selectAction_) {
        selectAction_->setChecked(true);
    }
    deactivateViewTools();
    canvas_->setInteractionMode(QVedWidget::InteractionMode::SelectObject);
    setActiveOperationStatus(QStringLiteral("VOM_SELECT_MOVE_NODE_OBJECT"));
    updateMouseToleranceCrossCursor();
}

void MainWindow::updateGroupCommandActions() {
    if (!model_) {
        return;
    }

    const int selectedCount = model_->CountSelectedObjects();
    bool hasSelectedGroup = false;
    for (TDVecObject* object : model_->GetSelectedObjects()) {
        if (object && object->GetType() == VECOBJ_GRP) {
            hasSelectedGroup = true;
            break;
        }
    }

    if (makeGroupAction_) {
        makeGroupAction_->setEnabled(selectedCount > 1);
    }
    if (resolveGroupAction_) {
        resolveGroupAction_->setEnabled(hasSelectedGroup);
    }
    for (QAction* alignAction : alignActions_) {
        if (alignAction) {
            alignAction->setEnabled(selectedCount > 1);
        }
    }
}

void MainWindow::updateClipboardCommandActions() {
    const bool hasSelection = model_ && model_->CountSelectedObjects() > 0;
    if (cutAction_) {
        cutAction_->setEnabled(hasSelection);
    }
    if (copyAction_) {
        copyAction_->setEnabled(hasSelection);
    }
    if (pasteAction_) {
        pasteAction_->setEnabled(operationManager_ && operationManager_->HasClipboardObjects());
    }
}

void MainWindow::updateHistoryCommandActions() {
    if (undoAction_) {
        undoAction_->setEnabled(operationManager_ && operationManager_->CanUndo());
    }
    if (redoAction_) {
        redoAction_->setEnabled(operationManager_ && operationManager_->CanRedo());
    }
}

void MainWindow::updateModifyCommandActions() {
    if (!model_) {
        return;
    }

    const int selectedCount = model_->CountSelectedObjects();
    const bool hasAnySelection = selectedCount > 0;
    const bool hasSingleSelection = selectedCount == 1;
    const TDVecObject* selectedObject = hasSingleSelection ? model_->GetSelectedObject() : nullptr;
    const bool canInsertDeleteNode = supportsInsertDeleteNode(selectedObject);
    const bool canMoveBsplineControlPoint = selectedObject && selectedObject->GetType() == VECOBJ_BSPLINE;
    bool canChangeEdgeRoundNode = false;
    bool canModifyCurveAttribute = false;
    bool canModifyText = false;
    if (selectedObject) {
        canChangeEdgeRoundNode = selectedObject->GetType() == VECOBJ_POLYCURVE ||
                                 selectedObject->GetType() == VECOBJ_POLYCURVEAREA;
        canModifyCurveAttribute = selectedObject->GetType() == VECOBJ_BSPLINE ||
                                  selectedObject->GetType() == VECOBJ_POLYCURVE;
        canModifyText = selectedObject->GetType() == VECOBJ_TEX ||
                        selectedObject->GetType() == VECOBJ_FRAMETEXT;
    }

    auto updateSingleSelectionAction = [this](QAction* action, bool enabled) {
        if (!action) {
            return;
        }
        action->setEnabled(enabled);
        if (!enabled && action->isChecked()) {
            action->setChecked(false);
            if (canvas_->interactionMode() == QVedWidget::InteractionMode::ModifyObject) {
                canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
                setActiveOperationStatus(QString());
            }
        }
    };

    updateSingleSelectionAction(moveObjectAction_, hasAnySelection);
    updateSingleSelectionAction(moveNodeAction_, hasSingleSelection);
    updateSingleSelectionAction(rotateObjectAction_, hasAnySelection);
    updateSingleSelectionAction(scaleObjectAction_, hasSingleSelection);
    updateSingleSelectionAction(insertNodeAction_, canInsertDeleteNode);
    updateSingleSelectionAction(deleteNodeAction_, canInsertDeleteNode);
    updateSingleSelectionAction(moveBsplineControlPointAction_, canMoveBsplineControlPoint);

    if (lockObjectAction_) {
        lockObjectAction_->setEnabled(hasAnySelection);
        lockObjectAction_->setChecked(
            hasAnySelection &&
            (model_->GetLockResizeForSelectedObjects() != 0 ||
             model_->GetLockPositionForSelectedObjects() != 0));
    }

    if (changeEdgeRoundNodeAction_) {
        updateSingleSelectionAction(changeEdgeRoundNodeAction_, canChangeEdgeRoundNode);
    }
    if (modifyCurveAttributeAction_) {
        updateSingleSelectionAction(modifyCurveAttributeAction_, canModifyCurveAttribute);
        if (!canModifyCurveAttribute && curveDock_ &&
            operationManager_ &&
            operationManager_->GetAktiveOperationType() == VOM_MODIFY_CURVE_ATTRIBUTE) {
            curveDock_->hide();
        }
    }
    if (modifyTextAction_) {
        updateSingleSelectionAction(modifyTextAction_, canModifyText);
        if (!canModifyText && textDockMode_ == TextDockMode::Modify) {
            cancelTextDockOperation();
        }
    }
    updateClipboardCommandActions();
    updateHistoryCommandActions();
}

void MainWindow::updateMouseToleranceCrossCursor() {
    if (!editor_ || !operationManager_ || !mouseToleranceCrossAction_) {
        return;
    }

    const TDViewOperationEnum activeOperation = operationManager_->GetAktiveOperationType();
    const bool selectOperation = activeOperation == VOM_SELECT_MOVE_NODE_OBJECT ||
                                 activeOperation == VOM_SELECTMOVE_OBJ_SCALE_FRAME;
    if (!selectOperation) {
        return;
    }

    const TDVecViewCursor usedCursor = editor_->GetUsedCursor();
    if (usedCursor == VECVIEW_DOCK || usedCursor == VECVIEW_NO) {
        return;
    }

    editor_->UseCursor(mouseToleranceCrossAction_->isChecked() ? VECVIEW_SIMPLE_CROSS : VECVIEW_CURSOR_DEFAULT);
}

void MainWindow::deactivateViewTools() {
    if (zoomAction_) {
        zoomAction_->setChecked(false);
    }
    if (panAction_) {
        panAction_->setChecked(false);
    }
    canvas_->setZoomToolEnabled(false);
    canvas_->setPanToolEnabled(false);
}

void MainWindow::setActiveOperationStatus(const QString& operationName) {
    hideCurveDockIfInactive();
    hideRoundRectangleDockIfInactive();
    hideTextDockIfInactive();

    if (!activeOperationLabel_) {
        return;
    }

    if (operationName.isEmpty()) {
        activeOperationLabel_->setText(QStringLiteral("Operation: -"));
        return;
    }

    activeOperationLabel_->setText(QStringLiteral("Operation: %1").arg(operationName));
}

void MainWindow::updateCoordinateStatus(TDMatPoint point, bool valid) {
    if (!coordinateLabel_) {
        return;
    }

    if (!valid) {
        coordinateLabel_->setText(QStringLiteral("X: -  Y: -"));
        return;
    }

    const TDVecUnitSettings unitSettings = model_
        ? model_->UnitSettings() : TDVecUnitSettings{};
    const TDVecUnitFormatter formatter(unitSettings);
    coordinateLabel_->setText(
        QStringLiteral("X: %1  Y: %2")
            .arg(QString::fromStdString(formatter.FormatCoordinate(point.x)))
            .arg(QString::fromStdString(formatter.FormatCoordinate(point.y))));
}

void MainWindow::exportPdf() {
    if (!model_) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export PDF"),
        dialogStartPath(currentDocumentPath_, lastDocumentDirectory_, QStringLiteral("untitled.pdf")),
        QStringLiteral("PDF files (*.pdf);;All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName += QStringLiteral(".pdf");
    }

    const auto& ds = model_->DocumentSettings();
    const auto& ps = ds.pageSettings;
    const TDVecUnitFormatter formatter(ds.unitSettings);
    const double pageWidthMm = formatter.ToMillimeters(ps.widthReal);
    const double pageHeightMm = formatter.ToMillimeters(ps.heightReal);

    QPdfWriter pdfWriter(fileName);
    pdfWriter.setResolution(200);
    pdfWriter.setPageSize(QPageSize(QSizeF(pageWidthMm, pageHeightMm), QPageSize::Millimeter));
    pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));

    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        QMessageBox::warning(
            this,
            QStringLiteral("Export PDF"),
            QStringLiteral("Could not create PDF file '%1'.").arg(fileName));
        return;
    }

    const int dpi = pdfWriter.resolution();
    const long pdfWidth = static_cast<long>(pageWidthMm / 25.4 * dpi);
    const long pdfHeight = static_cast<long>(pageHeightMm / 25.4 * dpi);

    TDGraphicEngineQt exportEngine;
    exportEngine.SetDeviceMetrics(pdfWidth, pdfHeight, dpi, dpi);
    exportEngine.SetWorkSpaceRange(
        ps.pageOriginX, ps.pageOriginY,
        ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal);
    exportEngine.SetWorldSpaceRange(
        ps.pageOriginX, ps.pageOriginY,
        ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal);
    exportEngine.SetViewRange(
        ps.pageOriginX, ps.pageOriginY,
        ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal);
    exportEngine.SetPainter(&painter);

    for (int i = 0; i < model_->CountObjects(); i++) {
        TDVecObject* obj = model_->GetObject(i);
        if (obj) {
            obj->Draw(&exportEngine, false);
        }
    }

    painter.end();
    statusBar()->showMessage(
        QStringLiteral("PDF exported: %1").arg(QFileInfo(fileName).fileName()), 2500);
}

void MainWindow::printDocument() {
    if (!model_) {
        return;
    }

    const auto& ds = model_->DocumentSettings();
    const auto& ps = ds.pageSettings;
    const TDVecUnitFormatter formatter(ds.unitSettings);
    const double pageWidthMm = formatter.ToMillimeters(ps.widthReal);
    const double pageHeightMm = formatter.ToMillimeters(ps.heightReal);

    const double shortSide = std::min(pageWidthMm, pageHeightMm);
    const double longSide = std::max(pageWidthMm, pageHeightMm);

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QSizeF(shortSide, longSide), QPageSize::Millimeter));
    if (pageWidthMm > pageHeightMm) {
        printer.setPageOrientation(QPageLayout::Landscape);
    }
    printer.setPageMargins(QMarginsF(0, 0, 0, 0));

    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QPainter painter(&printer);
    if (!painter.isActive()) {
        QMessageBox::warning(
            this,
            QStringLiteral("Print"),
            QStringLiteral("Could not start printing."));
        return;
    }

    const int dpi = printer.resolution();
    const long printWidth = static_cast<long>(pageWidthMm / 25.4 * dpi);
    const long printHeight = static_cast<long>(pageHeightMm / 25.4 * dpi);

    painter.setWindow(0, 0, static_cast<int>(printWidth), static_cast<int>(printHeight));
    painter.setViewport(0, 0, painter.device()->width(), painter.device()->height());

    TDGraphicEngineQt printEngine;
    printEngine.SetDeviceMetrics(printWidth, printHeight, dpi, dpi);
    printEngine.SetWorkSpaceRange(
        ps.pageOriginX, ps.pageOriginY,
        ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal);
    printEngine.SetWorldSpaceRange(
        ps.pageOriginX, ps.pageOriginY,
        ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal);
    printEngine.SetViewRange(
        ps.pageOriginX, ps.pageOriginY,
        ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal);
    printEngine.SetPainter(&painter);

    for (int i = 0; i < model_->CountObjects(); i++) {
        TDVecObject* obj = model_->GetObject(i);
        if (obj) {
            obj->Draw(&printEngine, false);
        }
    }

    painter.end();
    statusBar()->showMessage(QStringLiteral("Document printed"), 2500);
}

void MainWindow::onDocumentSetup() {
    if (!model_) {
        return;
    }

    DocumentSetupDialog dialog(model_->DocumentSettings(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    applyDocumentSettings(dialog.documentSettings());
    statusBar()->showMessage(QStringLiteral("Document setup changed"), 2500);
}

void MainWindow::applyDocumentSettings(const TDVecDocumentSettings& settings) {
    model_->SetDocumentSettings(settings);
    const auto& ps = settings.pageSettings;
    model_->SetTopLeftArea({ps.pageOriginX, ps.pageOriginY});
    model_->SetBottomRightArea({ps.pageOriginX + ps.widthReal, ps.pageOriginY + ps.heightReal});
    canvas_->resetView();
    updatePageFormatStatus();
}

void MainWindow::updatePageFormatStatus() {
    if (!pageFormatLabel_) {
        return;
    }

    if (!model_) {
        pageFormatLabel_->setText(QString());
        return;
    }

    const auto& ps = model_->PageSettings();
    const TDVecUnitFormatter formatter(model_->UnitSettings());
    const QString orientation = ps.orientation == TDVecPageOrientation::Portrait
        ? QStringLiteral("Portrait") : QStringLiteral("Landscape");
    const std::string widthStr = formatter.FormatLength(ps.widthReal);
    const std::string heightStr = formatter.FormatLength(ps.heightReal);
    pageFormatLabel_->setText(
        QStringLiteral("%1 %2 — %3 × %4")
            .arg(QString::fromStdString(ps.formatName))
            .arg(orientation)
            .arg(QString::fromStdString(widthStr))
            .arg(QString::fromStdString(heightStr)));
}

void MainWindow::updateWindowTitle() {
    if (currentDocumentPath_.isEmpty()) {
        setWindowTitle(QStringLiteral("VED Qt Port"));
    } else {
        setWindowTitle(QStringLiteral("%1 — VED Qt Port")
            .arg(QFileInfo(currentDocumentPath_).fileName()));
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!confirmDiscardUnsavedChanges()) {
        event->ignore();
        return;
    }

    writeSettings();
    QMainWindow::closeEvent(event);
}
