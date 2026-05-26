#include "DocumentSetupDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>

namespace {

constexpr int kFormatA4 = 0;
constexpr int kFormatA3 = 1;
constexpr int kFormatA5 = 2;
constexpr int kFormatLetter = 3;
constexpr int kFormatCustom = 4;

constexpr int kUnitMillimeter = 0;
constexpr int kUnitCentimeter = 1;
constexpr int kUnitInch = 2;

int formatIndexFromName(const std::string& name) {
    if (name == "A4") return kFormatA4;
    if (name == "A3") return kFormatA3;
    if (name == "A5") return kFormatA5;
    if (name == "Letter") return kFormatLetter;
    return kFormatCustom;
}

int unitIndexFromEnum(TDVecDisplayUnit unit) {
    switch (unit) {
    case TDVecDisplayUnit::Centimeter: return kUnitCentimeter;
    case TDVecDisplayUnit::Inch: return kUnitInch;
    default: return kUnitMillimeter;
    }
}

TDVecPageSettings pageSettingsForFormat(int index, TDVecPageOrientation orientation) {
    switch (index) {
    case kFormatA4: return TDVecPageFormats::A4(orientation);
    case kFormatA3: return TDVecPageFormats::A3(orientation);
    case kFormatA5: return TDVecPageFormats::A5(orientation);
    case kFormatLetter: return TDVecPageFormats::Letter(orientation);
    default: return {};
    }
}

TDVecGridSettings gridDefaultsForUnit(TDVecDisplayUnit unit) {
    TDVecGridSettings grid;
    if (unit == TDVecDisplayUnit::Inch) {
        grid.majorStepReal = 25400.0;
        grid.subdivisions = 8;
    } else {
        grid.majorStepReal = 10000.0;
        grid.subdivisions = 10;
    }
    return grid;
}

} // namespace

DocumentSetupDialog::DocumentSetupDialog(const TDVecDocumentSettings& currentSettings,
                                          QWidget* parent)
    : QDialog(parent),
      unitSettings_(currentSettings.unitSettings)
{
    setWindowTitle(QStringLiteral("Document Setup"));
    setMinimumWidth(350);

    auto* mainLayout = new QVBoxLayout(this);

    auto* topForm = new QFormLayout;
    unitCombo_ = new QComboBox(this);
    unitCombo_->addItems({
        QStringLiteral("Millimeter (mm)"),
        QStringLiteral("Centimeter (cm)"),
        QStringLiteral("Inch (in)")
    });
    topForm->addRow(QStringLiteral("Unit:"), unitCombo_);

    formatCombo_ = new QComboBox(this);
    formatCombo_->addItems({
        QStringLiteral("A4"),
        QStringLiteral("A3"),
        QStringLiteral("A5"),
        QStringLiteral("Letter"),
        QStringLiteral("Custom")
    });
    topForm->addRow(QStringLiteral("Format:"), formatCombo_);

    orientationCombo_ = new QComboBox(this);
    orientationCombo_->addItems({
        QStringLiteral("Portrait"),
        QStringLiteral("Landscape")
    });
    topForm->addRow(QStringLiteral("Orientation:"), orientationCombo_);
    mainLayout->addLayout(topForm);

    const QString suffix = QStringLiteral(" ") + unitSuffix();
    const double minDim = toDisplay(1000.0);
    const double maxDim = toDisplay(5000000.0);

    auto* dimGroup = new QGroupBox(QStringLiteral("Dimensions"), this);
    auto* dimForm = new QFormLayout(dimGroup);

    widthSpin_ = new QDoubleSpinBox(this);
    widthSpin_->setRange(minDim, maxDim);
    widthSpin_->setDecimals(2);
    widthSpin_->setSuffix(suffix);
    dimForm->addRow(QStringLiteral("Width:"), widthSpin_);

    heightSpin_ = new QDoubleSpinBox(this);
    heightSpin_->setRange(minDim, maxDim);
    heightSpin_->setDecimals(2);
    heightSpin_->setSuffix(suffix);
    dimForm->addRow(QStringLiteral("Height:"), heightSpin_);
    mainLayout->addWidget(dimGroup);

    const double minOrigin = toDisplay(-10000000.0);
    const double maxOrigin = toDisplay(10000000.0);

    auto* originGroup = new QGroupBox(QStringLiteral("Position in Drawing Area"), this);
    auto* originForm = new QFormLayout(originGroup);

    originXSpin_ = new QDoubleSpinBox(this);
    originXSpin_->setRange(minOrigin, maxOrigin);
    originXSpin_->setDecimals(2);
    originXSpin_->setSuffix(suffix);
    originForm->addRow(QStringLiteral("Origin X:"), originXSpin_);

    originYSpin_ = new QDoubleSpinBox(this);
    originYSpin_->setRange(minOrigin, maxOrigin);
    originYSpin_->setDecimals(2);
    originYSpin_->setSuffix(suffix);
    originForm->addRow(QStringLiteral("Origin Y:"), originYSpin_);
    mainLayout->addWidget(originGroup);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

    unitCombo_->setCurrentIndex(unitIndexFromEnum(currentSettings.unitSettings.displayUnit));
    formatCombo_->setCurrentIndex(formatIndexFromName(currentSettings.pageSettings.formatName));
    orientationCombo_->setCurrentIndex(
        currentSettings.pageSettings.orientation == TDVecPageOrientation::Landscape ? 1 : 0);
    widthSpin_->setValue(toDisplay(currentSettings.pageSettings.widthReal));
    heightSpin_->setValue(toDisplay(currentSettings.pageSettings.heightReal));
    originXSpin_->setValue(toDisplay(currentSettings.pageSettings.pageOriginX));
    originYSpin_->setValue(toDisplay(currentSettings.pageSettings.pageOriginY));
    updateDimensionReadOnly();

    connect(unitCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DocumentSetupDialog::onUnitChanged);
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DocumentSetupDialog::onFormatChanged);
    connect(orientationCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DocumentSetupDialog::onOrientationChanged);
}

TDVecDocumentSettings DocumentSetupDialog::documentSettings() const {
    TDVecDocumentSettings settings;
    settings.unitSettings.displayUnit = selectedUnit();
    settings.unitSettings.realUnitsPerMillimeter = unitSettings_.realUnitsPerMillimeter;
    settings.gridSettings = gridDefaultsForUnit(selectedUnit());

    auto& ps = settings.pageSettings;
    switch (formatCombo_->currentIndex()) {
    case kFormatA4: ps.formatName = "A4"; break;
    case kFormatA3: ps.formatName = "A3"; break;
    case kFormatA5: ps.formatName = "A5"; break;
    case kFormatLetter: ps.formatName = "Letter"; break;
    default: ps.formatName = "Custom"; break;
    }
    ps.orientation = orientationCombo_->currentIndex() == 1
        ? TDVecPageOrientation::Landscape : TDVecPageOrientation::Portrait;
    ps.widthReal = fromDisplay(widthSpin_->value());
    ps.heightReal = fromDisplay(heightSpin_->value());
    ps.pageOriginX = fromDisplay(originXSpin_->value());
    ps.pageOriginY = fromDisplay(originYSpin_->value());
    return settings;
}

void DocumentSetupDialog::onUnitChanged(int index) {
    Q_UNUSED(index);

    const double oldWidth = fromDisplay(widthSpin_->value());
    const double oldHeight = fromDisplay(heightSpin_->value());
    const double oldOriginX = fromDisplay(originXSpin_->value());
    const double oldOriginY = fromDisplay(originYSpin_->value());

    unitSettings_.displayUnit = selectedUnit();
    updateSpinBoxUnit();

    widthSpin_->setValue(toDisplay(oldWidth));
    heightSpin_->setValue(toDisplay(oldHeight));
    originXSpin_->setValue(toDisplay(oldOriginX));
    originYSpin_->setValue(toDisplay(oldOriginY));
}

void DocumentSetupDialog::onFormatChanged(int index) {
    if (index == kFormatCustom) {
        updateDimensionReadOnly();
        return;
    }

    const TDVecPageOrientation orientation = orientationCombo_->currentIndex() == 1
        ? TDVecPageOrientation::Landscape : TDVecPageOrientation::Portrait;
    const TDVecPageSettings preset = pageSettingsForFormat(index, orientation);
    widthSpin_->setValue(toDisplay(preset.widthReal));
    heightSpin_->setValue(toDisplay(preset.heightReal));
    updateDimensionReadOnly();
}

void DocumentSetupDialog::onOrientationChanged(int index) {
    Q_UNUSED(index);

    if (formatCombo_->currentIndex() != kFormatCustom) {
        const TDVecPageOrientation orientation = index == 1
            ? TDVecPageOrientation::Landscape : TDVecPageOrientation::Portrait;
        const TDVecPageSettings preset = pageSettingsForFormat(
            formatCombo_->currentIndex(), orientation);
        widthSpin_->setValue(toDisplay(preset.widthReal));
        heightSpin_->setValue(toDisplay(preset.heightReal));
        return;
    }

    const double oldWidth = widthSpin_->value();
    const double oldHeight = heightSpin_->value();
    widthSpin_->setValue(oldHeight);
    heightSpin_->setValue(oldWidth);
}

void DocumentSetupDialog::updateDimensionReadOnly() {
    const bool isCustom = formatCombo_->currentIndex() == kFormatCustom;
    widthSpin_->setReadOnly(!isCustom);
    heightSpin_->setReadOnly(!isCustom);
    widthSpin_->setButtonSymbols(
        isCustom ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
    heightSpin_->setButtonSymbols(
        isCustom ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
}

void DocumentSetupDialog::updateSpinBoxUnit() {
    const QString suffix = QStringLiteral(" ") + unitSuffix();
    const double minDim = toDisplay(1000.0);
    const double maxDim = toDisplay(5000000.0);
    const double minOrigin = toDisplay(-10000000.0);
    const double maxOrigin = toDisplay(10000000.0);

    for (auto* spin : {widthSpin_, heightSpin_}) {
        spin->setSuffix(suffix);
        spin->setRange(minDim, maxDim);
    }
    for (auto* spin : {originXSpin_, originYSpin_}) {
        spin->setSuffix(suffix);
        spin->setRange(minOrigin, maxOrigin);
    }
}

TDVecDisplayUnit DocumentSetupDialog::selectedUnit() const {
    switch (unitCombo_->currentIndex()) {
    case kUnitCentimeter: return TDVecDisplayUnit::Centimeter;
    case kUnitInch: return TDVecDisplayUnit::Inch;
    default: return TDVecDisplayUnit::Millimeter;
    }
}

double DocumentSetupDialog::toDisplay(double realValue) const {
    return TDVecUnitFormatter(unitSettings_).ToDisplayUnit(realValue);
}

double DocumentSetupDialog::fromDisplay(double displayValue) const {
    return TDVecUnitFormatter(unitSettings_).FromDisplayUnit(displayValue);
}

QString DocumentSetupDialog::unitSuffix() const {
    switch (unitSettings_.displayUnit) {
    case TDVecDisplayUnit::Millimeter: return QStringLiteral("mm");
    case TDVecDisplayUnit::Centimeter: return QStringLiteral("cm");
    case TDVecDisplayUnit::Inch: return QStringLiteral("in");
    case TDVecDisplayUnit::RawVed: return QStringLiteral("ved");
    }
    return {};
}
