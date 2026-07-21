#include "MainWindow.h"

#include "QtVecFontProviders.h"
#include "QVedWidget.h"
#include "vec_edit_cad.h"
#include "vec_font.h"
#include "vec_font_manager.h"
#include "vec_model.h"
#include "vec_text.h"
#include "voc_bspline_controlpoint.h"
#include "voc_polycurve.h"
#include "voc_roundrectangle_notrotated.h"
#include "voc_vecframetext_empty.h"
#include "voc_vectext.h"
#include "vom_modify_curve_attribute.h"
#include "vom_rotate_object_activepoint.h"
#include "vom_vectext.h"
#include "vop_manager.h"

#include <QAction>
#include <QActionGroup>
#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QDoubleValidator>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QTimer>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <Qt>

#include <cstdio>
#include <string>

namespace {
constexpr int kFontIdRole = Qt::UserRole;
constexpr int kFontSearchRole = Qt::UserRole + 1;

class FontComboBox final : public QComboBox {
public:
    explicit FontComboBox(QWidget* parent = nullptr)
        : QComboBox(parent) {
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void showPopup() override {
        QComboBox::showPopup();
        popupView_ = QComboBox::view();
        if (popupView_) {
            popupView_->installEventFilter(this);
        }
    }

    bool event(QEvent* event) override {
        if (event && event->type() == QEvent::ShortcutOverride) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (!keyEvent->text().isEmpty() && keyEvent->text().at(0).isPrint()) {
                event->accept();
                return true;
            }
        }
        return QComboBox::event(event);
    }

    void keyPressEvent(QKeyEvent* event) override {
        if (handleSearchKey(event)) {
            return;
        }
        QComboBox::keyPressEvent(event);
    }

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == popupView_ && event && event->type() == QEvent::KeyPress) {
            if (handleSearchKey(static_cast<QKeyEvent*>(event))) {
                return true;
            }
        }
        if (watched == popupView_ && event && event->type() == QEvent::ShortcutOverride) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (!keyEvent->text().isEmpty() && keyEvent->text().at(0).isPrint()) {
                event->accept();
                return true;
            }
        }
        return QComboBox::eventFilter(watched, event);
    }

private:
    bool handleSearchKey(QKeyEvent* event) {
        if (event && !event->text().isEmpty() && event->text().at(0).isPrint()) {
            searchText_ += event->text();
            const int index = findBySearchText(searchText_);
            if (index >= 0) {
                selectSearchIndex(index);
                return true;
            }
            searchText_ = event->text();
            const int singleCharIndex = findBySearchText(searchText_);
            if (singleCharIndex >= 0) {
                selectSearchIndex(singleCharIndex);
                return true;
            }
        } else if (event && event->key() == Qt::Key_Backspace) {
            searchText_.chop(1);
            return true;
        } else if (event && event->key() == Qt::Key_Escape) {
            searchText_.clear();
            return false;
        }
        return false;
    }

    void selectSearchIndex(int index) {
        setCurrentIndex(index);
        if (popupView_ && popupView_->isVisible()) {
            popupView_->setCurrentIndex(model()->index(index, modelColumn(), rootModelIndex()));
            popupView_->scrollTo(popupView_->currentIndex());
        }
        QTimer::singleShot(1200, this, [this] {
            searchText_.clear();
        });
    }

    int findBySearchText(const QString& value) const {
        if (value.isEmpty()) {
            return -1;
        }
        for (int i = 0; i < count(); ++i) {
            if (itemData(i, kFontSearchRole).toString().startsWith(value, Qt::CaseInsensitive)) {
                return i;
            }
        }
        return -1;
    }

    QString searchText_;
    QAbstractItemView* popupView_ = nullptr;
};

TDVecText::TDJustification justificationFromIndex(int index) {
    switch (index) {
    case 1:
        return TDVecText::JUST_CENTER;
    case 2:
        return TDVecText::JUST_RIGHT;
    default:
        return TDVecText::JUST_LEFT;
    }
}

int indexFromJustification(TDVecText::TDJustification justification) {
    switch (justification) {
    case TDVecText::JUST_CENTER:
        return 1;
    case TDVecText::JUST_RIGHT:
        return 2;
    default:
        return 0;
    }
}

TDVecText::TDVerticalAlignment verticalAlignmentFromIndex(int index) {
    switch (index) {
    case 1:
        return TDVecText::VALIGN_CENTER;
    case 2:
        return TDVecText::VALIGN_BOTTOM;
    default:
        return TDVecText::VALIGN_TOP;
    }
}

int indexFromVerticalAlignment(TDVecText::TDVerticalAlignment alignment) {
    switch (alignment) {
    case TDVecText::VALIGN_CENTER:
        return 1;
    case TDVecText::VALIGN_BOTTOM:
        return 2;
    default:
        return 0;
    }
}

QString textFromStoredText(const char* text) {
    if (!text) {
        return {};
    }
    const QString utf8 = QString::fromUtf8(text);
    return utf8.contains(QChar::ReplacementCharacter) ? QString::fromLatin1(text) : utf8;
}

} // namespace

void MainWindow::createTextDock() {
    textDock_ = new QDockWidget(QStringLiteral("Text"), this);
    textDock_->setObjectName(QStringLiteral("TextDock"));
    textDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* panel = new QWidget(textDock_);
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

    textEdit_ = new QPlainTextEdit(panel);
    textEdit_->setPlainText(QStringLiteral("VED Text"));
    textEdit_->setMinimumHeight(72);
    rootLayout->addWidget(textEdit_);

    textFontCombo_ = new FontComboBox(panel);
    textFontCombo_->addItem(QStringLiteral("WPS Default"));
    textFontCombo_->setItemData(0, QStringLiteral("VC:WPS Default"), kFontIdRole);
    textFontCombo_->setItemData(0, QStringLiteral("WPS Default"), kFontSearchRole);
    textFontCombo_->setEnabled(true);

    textXScaleSpin_ = new QDoubleSpinBox(panel);
    textXScaleSpin_->setRange(0.05, 100.0);
    textXScaleSpin_->setDecimals(3);
    textXScaleSpin_->setValue(1.0);

    textYScaleSpin_ = new QDoubleSpinBox(panel);
    textYScaleSpin_->setRange(0.05, 100.0);
    textYScaleSpin_->setDecimals(3);
    textYScaleSpin_->setValue(1.0);

    textAngleSpin_ = new QDoubleSpinBox(panel);
    textAngleSpin_->setRange(-360.0, 360.0);
    textAngleSpin_->setDecimals(2);

    textInclineSpin_ = new QDoubleSpinBox(panel);
    textInclineSpin_->setRange(-89.0, 89.0);
    textInclineSpin_->setDecimals(2);

    textLineSpacingSpin_ = new QDoubleSpinBox(panel);
    textLineSpacingSpin_->setRange(-10000.0, 10000.0);
    textLineSpacingSpin_->setDecimals(2);
    textLineSpacingSpin_->setValue(1.0);

    textCharSpacingSpin_ = new QDoubleSpinBox(panel);
    textCharSpacingSpin_->setRange(-10000.0, 10000.0);
    textCharSpacingSpin_->setDecimals(2);
    textCharSpacingSpin_->setValue(1.0);

    textResolutionSpin_ = new QSpinBox(panel);
    textResolutionSpin_->setRange(1, 8);
    textResolutionSpin_->setValue(4);

    textJustificationCombo_ = new QComboBox(panel);
    textJustificationCombo_->addItems({QStringLiteral("Left"), QStringLiteral("Center"), QStringLiteral("Right")});

    textVerticalAlignCombo_ = new QComboBox(panel);
    textVerticalAlignCombo_->addItems({QStringLiteral("Top"), QStringLiteral("Center"), QStringLiteral("Bottom")});

    auto* formLayout = new QFormLayout();
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(QStringLiteral("font:"), textFontCombo_);
    formLayout->addRow(QStringLiteral("x scale:"), textXScaleSpin_);
    formLayout->addRow(QStringLiteral("y scale:"), textYScaleSpin_);
    formLayout->addRow(QStringLiteral("angle:"), textAngleSpin_);
    formLayout->addRow(QStringLiteral("incline:"), textInclineSpin_);
    formLayout->addRow(QStringLiteral("line spacing:"), textLineSpacingSpin_);
    formLayout->addRow(QStringLiteral("char spacing:"), textCharSpacingSpin_);
    formLayout->addRow(QStringLiteral("resolution:"), textResolutionSpin_);
    formLayout->addRow(QStringLiteral("justify:"), textJustificationCombo_);
    formLayout->addRow(QStringLiteral("vertical align:"), textVerticalAlignCombo_);
    rootLayout->addLayout(formLayout);

    textVerticalCheck_ = new QCheckBox(QStringLiteral("vertical"), panel);
    textUnderlineCheck_ = new QCheckBox(QStringLiteral("underline"), panel);
    textScaleDependencyCheck_ = new QCheckBox(QStringLiteral("scale with frame"), panel);
    textScaleDependencyCheck_->setChecked(true);
    rootLayout->addWidget(textVerticalCheck_);
    rootLayout->addWidget(textUnderlineCheck_);
    rootLayout->addWidget(textScaleDependencyCheck_);
    rootLayout->addStretch();

    textDock_->setWidget(panel);
    textDock_->hide();
    addDockWidget(Qt::RightDockWidgetArea, textDock_);

    connect(applyButton, &QToolButton::clicked, this, &MainWindow::applyTextSettings);
    connect(cancelButton, &QToolButton::clicked, this, &MainWindow::cancelTextDockOperation);
    connect(textEdit_, &QPlainTextEdit::textChanged, this, [this] {
        if (textDockMode_ == TextDockMode::CreateText) {
            updateCreateTextExtVar(false);
        }
    });
    connect(textFontCombo_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (textDockMode_ == TextDockMode::CreateText) {
            updateCreateTextExtVar(false);
        }
    });
    connect(textDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        if (!visible && textDockMode_ != TextDockMode::None && textDockMode_ != TextDockMode::CreateFrameTextBox) {
            cancelTextDockOperation();
        }
    });
}

void MainWindow::showTextDockForCreate(bool frameText) {
    if (!textDock_) {
        return;
    }

    deactivateViewTools();
    if (!loadDefaultVecFont()) {
        QApplication::beep();
        return;
    }
    if (editor_) {
        editor_->SetOperation(VOC_VECTEXT);
    }
    canvas_->setInteractionMode(QVedWidget::InteractionMode::CreateLine);

    (void)frameText;
    textDockMode_ = TextDockMode::CreateText;
    textDock_->setWindowTitle(QStringLiteral("Text erzeugen"));
    textScaleDependencyCheck_->setVisible(false);
    textScaleDependencyCheck_->setEnabled(false);
    textScaleDependencyCheck_->setChecked(true);
    textDock_->show();
    textDock_->raise();
    updateCreateTextExtVar(false);
    setActiveOperationStatus(QStringLiteral("cmdVOCVecText"));
    statusBar()->showMessage(QStringLiteral("cmdVOCVecText"), 2500);
    canvas_->update();
}

void MainWindow::showTextDockForModify() {
    if (!textDock_ || !model_ || model_->CountSelectedObjects() != 1) {
        return;
    }

    auto* text = dynamic_cast<TDVecText*>(model_->GetSelectedObject());
    if (!text) {
        return;
    }

    deactivateViewTools();
    if (!loadDefaultVecFont()) {
        QApplication::beep();
        return;
    }
    if (editor_) {
        editor_->SetOperation(VOM_VECTEXT);
    }
    canvas_->setInteractionMode(QVedWidget::InteractionMode::ModifyObject);

    auto* extVar = operationManager_
        ? dynamic_cast<TDVOMVecTextExtVar*>(operationManager_->GetActiveVOPExtVariables())
        : nullptr;
    if (!extVar) {
        return;
    }

    TDVecText::TDVecTextParameter params;
    extVar->GetParameter(&params);
    textEdit_->setPlainText(textFromStoredText(extVar->GetText()));
    textXScaleSpin_->setValue(params.mnXScale);
    textYScaleSpin_->setValue(params.mnYScale);
    textAngleSpin_->setValue(params.mnAngle);
    textInclineSpin_->setValue(params.mnIncline);
    textLineSpacingSpin_->setValue(params.mnLineSpacing);
    textCharSpacingSpin_->setValue(params.mnCharSpacing);
    textResolutionSpin_->setValue(static_cast<int>(params.mnResolution));
    textJustificationCombo_->setCurrentIndex(indexFromJustification(params.meJustification));
    textVerticalAlignCombo_->setCurrentIndex(indexFromVerticalAlignment(params.meVerticalAlignment));
    setTextFontComboToFontName(params.msFontName);
    textVerticalCheck_->setChecked(params.mbVertical);
    textUnderlineCheck_->setChecked(params.mbUnderline);

    const bool isFrameText = extVar->GetObjectType() == VECOBJ_FRAMETEXT;
    textScaleDependencyCheck_->setVisible(isFrameText);
    textScaleDependencyCheck_->setEnabled(isFrameText);
    textScaleDependencyCheck_->setChecked(params.mbScaleDependency);

    textDockMode_ = TextDockMode::Modify;
    textDock_->setWindowTitle(isFrameText ? QStringLiteral("Frame Text ändern") : QStringLiteral("Text ändern"));
    textDock_->show();
    textDock_->raise();
    setActiveOperationStatus(QStringLiteral("VOM_VECTEXT"));
}

void MainWindow::applyTextSettings() {
    if (!textDock_) {
        return;
    }

    if (textDockMode_ == TextDockMode::CreateText) {
        updateCreateTextExtVar(true);
        return;
    }

    if (textDockMode_ != TextDockMode::Modify || !operationManager_) {
        return;
    }

    auto* extVar = dynamic_cast<TDVOMVecTextExtVar*>(operationManager_->GetActiveVOPExtVariables());
    if (!extVar) {
        return;
    }
    const TDVecFont* selectedFont = currentTextVecFont();
    if (!selectedFont) {
        QApplication::beep();
        return;
    }

    TDVecText::TDVecTextParameter params;
    params.mnXScale = textXScaleSpin_->value();
    params.mnYScale = textYScaleSpin_->value();
    params.mnAngle = textAngleSpin_->value();
    params.mnIncline = textInclineSpin_->value();
    params.mnLineSpacing = textLineSpacingSpin_->value();
    params.mnCharSpacing = textCharSpacingSpin_->value();
    params.mnResolution = static_cast<unsigned int>(textResolutionSpin_->value());
    params.meJustification = justificationFromIndex(textJustificationCombo_->currentIndex());
    params.meVerticalAlignment = verticalAlignmentFromIndex(textVerticalAlignCombo_->currentIndex());
    params.mbVertical = textVerticalCheck_->isChecked();
    params.mbUnderline = textUnderlineCheck_->isChecked();
    params.mbScaleDependency = textScaleDependencyCheck_->isChecked();
    params.mpVecFont = selectedFont;
    std::snprintf(params.msFontName, sizeof(params.msFontName), "%s", selectedFont->GetFontName());

    const QByteArray textBytes = textEdit_->toPlainText().toUtf8();
    TDVecTextParameterValidity validity;
    validity.mbXScale = true;
    validity.mbYScale = true;
    validity.mbAngle = true;
    validity.mbIncline = true;
    validity.mbLineSpacing = true;
    validity.mbCharSpacing = true;
    validity.mbVertical = true;
    validity.mbUnderline = true;
    validity.mbJustification = true;
    validity.mbVerticalAlignment = true;
    validity.mbResolution = true;
    validity.mbVecFont = true;
    validity.mbScaleDependency = true;
    extVar->SetParameter(&params);
    extVar->SetParameterValidity(&validity);
    extVar->SetText(textBytes.constData());
    extVar->SendUpdateToParrentOP();
    canvas_->update();
    updateModifyCommandActions();
    statusBar()->showMessage(QStringLiteral("VOM_VECTEXT settings applied"), 1500);
}

bool MainWindow::updateCreateTextExtVar(bool showMessage) {
    if (textDockMode_ != TextDockMode::CreateText || !operationManager_) {
        return false;
    }
    const TDVecFont* selectedFont = currentTextVecFont();
    if (!selectedFont) {
        QApplication::beep();
        return false;
    }

    auto* extVar = dynamic_cast<TDVOCVecTextExtVar*>(operationManager_->GetActiveVOPExtVariables());
    if (!extVar) {
        return false;
    }

    TDVecText::TDVecTextParameter params;
    params.mnXScale = textXScaleSpin_->value();
    params.mnYScale = textYScaleSpin_->value();
    params.mnAngle = textAngleSpin_->value();
    params.mnIncline = textInclineSpin_->value();
    params.mnLineSpacing = textLineSpacingSpin_->value();
    params.mnCharSpacing = textCharSpacingSpin_->value();
    params.mnResolution = static_cast<unsigned int>(textResolutionSpin_->value());
    params.meJustification = justificationFromIndex(textJustificationCombo_->currentIndex());
    params.meVerticalAlignment = verticalAlignmentFromIndex(textVerticalAlignCombo_->currentIndex());
    params.mbVertical = textVerticalCheck_->isChecked();
    params.mbUnderline = textUnderlineCheck_->isChecked();
    params.mbScaleDependency = textScaleDependencyCheck_->isChecked();
    params.mpVecFont = selectedFont;
    std::snprintf(params.msFontName, sizeof(params.msFontName), "%s", selectedFont->GetFontName());

    const QByteArray textBytes = textEdit_->toPlainText().toUtf8();
    extVar->SetParameter(&params);
    extVar->SetText(textBytes.constData());
    extVar->SendUpdateToParrentOP();
    canvas_->update();
    if (showMessage) {
        statusBar()->showMessage(QStringLiteral("cmdVOCVecText settings applied"), 1500);
    }
    return true;
}

void MainWindow::cancelTextDockOperation() {
    const TextDockMode previousMode = textDockMode_;
    textDockMode_ = TextDockMode::None;

    if (createTextAction_) {
        createTextAction_->setChecked(false);
    }
    if (createFrameTextAction_) {
        createFrameTextAction_->setChecked(false);
    }
    if (modifyTextAction_) {
        modifyTextAction_->setChecked(false);
    }
    if (textDock_ && textDock_->isVisible()) {
        textDock_->hide();
    }

    if (previousMode == TextDockMode::CreateText ||
        previousMode == TextDockMode::CreateFrameTextBox ||
        (previousMode == TextDockMode::Modify &&
         operationManager_ &&
         operationManager_->GetAktiveOperationType() == VOM_VECTEXT)) {
        if (canvas_->interactionMode() == QVedWidget::InteractionMode::CreateLine) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::None);
        }
        if (editor_) {
            editor_->SetOperation(VOM_SELECT_MOVE_NODE_OBJECT);
        }
        if (previousMode == TextDockMode::Modify) {
            canvas_->setInteractionMode(QVedWidget::InteractionMode::SelectObject);
        }
        setActiveOperationStatus(QString());
        statusBar()->showMessage(QStringLiteral("Text operation canceled"), 1500);
    }
}

void MainWindow::hideTextDockIfInactive() {
    if (!textDock_) {
        return;
    }
    const bool textActionChecked = (createTextAction_ && createTextAction_->isChecked()) ||
                                   (createFrameTextAction_ && createFrameTextAction_->isChecked()) ||
                                   (modifyTextAction_ && modifyTextAction_->isChecked());
    if (!textActionChecked && textDockMode_ != TextDockMode::None) {
        cancelTextDockOperation();
    }
}

void MainWindow::startFrameTextBoxCreate() {
    deactivateViewTools();
    if (!loadDefaultVecFont()) {
        QApplication::beep();
        return;
    }
    if (editor_) {
        editor_->SetOperation(VOC_VECFRAMETEXT_EMPTY);
    }
    textDockMode_ = TextDockMode::CreateFrameTextBox;
    if (textDock_) {
        textDock_->hide();
    }
    canvas_->setInteractionMode(QVedWidget::InteractionMode::CreateLine);
    if (operationManager_) {
        auto* extVar = dynamic_cast<TDVOCVecFrameTextEmptyExtVar*>(operationManager_->GetActiveVOPExtVariables());
        if (extVar) {
            TDVecText::TDVecTextParameter params;
            const TDVecFont* selectedFont = currentTextVecFont();
            if (!selectedFont) {
                QApplication::beep();
                return;
            }
            params.mpVecFont = selectedFont;
            params.mbScaleDependency = true;
            std::snprintf(params.msFontName, sizeof(params.msFontName), "%s", selectedFont->GetFontName());
            extVar->SetParameter(&params);
            extVar->SendUpdateToParrentOP();
        }
    }
    setActiveOperationStatus(QStringLiteral("cmdVOCVecFrameTextEmpty"));
    statusBar()->showMessage(QStringLiteral("cmdVOCVecFrameTextEmpty"), 2500);
}

bool MainWindow::loadDefaultVecFont() {
    if (fontManager_ && fontManager_->GetDefaultVecFont()) {
        return true;
    }

    fontManager_ = std::make_unique<TDFontManager>();
    fontProviders_.clear();
    fontProviders_.push_back(std::make_unique<TDBuiltinVfnFontProvider>(
        QStringLiteral("VC:WPS Default"),
        QStringLiteral("WPS Default"),
        QStringLiteral(":/ved/font/wps_default.vfn")));

    // Auto-register every other bundled .vfn font, using the name embedded in
    // the file. wps_default is a legacy font without an embedded name, so it
    // stays explicit above and is skipped here.
    const QDir bundledFontDir(QStringLiteral(":/ved/font"));
    const QStringList bundledFonts =
        bundledFontDir.entryList({QStringLiteral("*.vfn")}, QDir::Files, QDir::Name);
    for (const QString& fileName : bundledFonts) {
        if (fileName == QStringLiteral("wps_default.vfn")) {
            continue;
        }
        const QString resourcePath = QStringLiteral(":/ved/font/") + fileName;
        QFile file(resourcePath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QByteArray data = file.readAll();
        const std::string embeddedName =
            PeekVfnFontName(data.constData(), static_cast<long>(data.size()));
        if (embeddedName.empty()) {
            continue;
        }
        QString fontId = QString::fromUtf8(embeddedName.c_str());
        QString displayName = fontId;
        if (displayName.startsWith(QStringLiteral("VC:")) ||
            displayName.startsWith(QStringLiteral("TT:"))) {
            displayName = displayName.mid(3);
        }
        fontProviders_.push_back(
            std::make_unique<TDBuiltinVfnFontProvider>(fontId, displayName, resourcePath));
    }

    auto systemFontProvider = std::make_unique<TDQtSystemFontProvider>();
    auto* systemFontProviderPtr = systemFontProvider.get();
    fontProviders_.push_back(std::move(systemFontProvider));
    for (const std::unique_ptr<IVecFontProvider>& provider : fontProviders_) {
        fontManager_->RegisterProvider(provider.get());
    }
    SetVecTextShaper([systemFontProviderPtr](const TDVecFont* font, const char* text, std::vector<TDVecShapedGlyph>& glyphs) {
        return systemFontProviderPtr->ShapeText(font, text, glyphs);
    });

    if (!fontManager_->SetDefaultFontId("VC:WPS Default")) {
        statusBar()->showMessage(QStringLiteral("wps_default.vfn could not be loaded"), 3000);
        return false;
    }

    SetDefaultVecTextFont(fontManager_->GetDefaultVecFont());
    populateTextFontCombo();
    return true;
}

void MainWindow::populateTextFontCombo()
{
    if (!textFontCombo_ || !fontManager_ || textFontCombo_->count() > 1) {
        return;
    }

    const QString currentFontId = textFontCombo_->currentData(kFontIdRole).toString();
    textFontCombo_->blockSignals(true);
    textFontCombo_->clear();
    const std::vector<TDVecFontDescriptor> fonts = fontManager_->AvailableFonts();
    for (const TDVecFontDescriptor& font : fonts) {
        const QString displayName = QString::fromUtf8(font.displayName);
        const QString fontId = QString::fromUtf8(font.fontId);
        textFontCombo_->addItem(font.isSystemFont ? QStringLiteral("TT: %1").arg(displayName) : displayName);
        const int index = textFontCombo_->count() - 1;
        textFontCombo_->setItemData(index, fontId, kFontIdRole);
        textFontCombo_->setItemData(index, displayName, kFontSearchRole);
    }
    const int selectedIndex = textFontCombo_->findData(currentFontId.isEmpty() ? QStringLiteral("VC:WPS Default") : currentFontId, kFontIdRole);
    textFontCombo_->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
    textFontCombo_->blockSignals(false);
}

const TDVecFont* MainWindow::currentTextVecFont()
{
    if (!loadDefaultVecFont() || !fontManager_) {
        return nullptr;
    }
    populateTextFontCombo();
    const QString fontId = textFontCombo_ ? textFontCombo_->currentData(kFontIdRole).toString() : QStringLiteral("VC:WPS Default");
    const QByteArray fontIdBytes = fontId.toUtf8();
    TDVecFont* font = fontManager_->GetVecFont({fontIdBytes.constData(), static_cast<std::size_t>(fontIdBytes.size())});
    if (!font) {
        return fontManager_->GetDefaultVecFont();
    }
    return font;
}

void MainWindow::setTextFontComboToFontName(const char* fontName)
{
    if (!textFontCombo_ || !fontName || !loadDefaultVecFont()) {
        return;
    }
    populateTextFontCombo();
    const int index = textFontCombo_->findData(QString::fromUtf8(fontName), kFontIdRole);
    textFontCombo_->setCurrentIndex(index >= 0 ? index : 0);
}
