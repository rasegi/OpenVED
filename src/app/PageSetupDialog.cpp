#include "PageSetupDialog.h"

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

int formatIndexFromName(const std::string& name) {
    if (name == "A4") return kFormatA4;
    if (name == "A3") return kFormatA3;
    if (name == "A5") return kFormatA5;
    if (name == "Letter") return kFormatLetter;
    return kFormatCustom;
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

} // namespace

PageSetupDialog::PageSetupDialog(const TDVecPageSettings& currentSettings,
                                  const TDVecUnitSettings& unitSettings,
                                  QWidget* parent)
    : QDialog(parent),
      formatter_(unitSettings)
{
    setWindowTitle(QStringLiteral("Page Setup"));
    setMinimumWidth(350);

    auto* mainLayout = new QVBoxLayout(this);

    auto* topForm = new QFormLayout;
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
    const double minDim = formatter_.ToDisplayUnit(formatter_.FromMillimeters(1.0));
    const double maxDim = formatter_.ToDisplayUnit(formatter_.FromMillimeters(5000.0));

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

    const double minOrigin = formatter_.ToDisplayUnit(formatter_.FromMillimeters(-10000.0));
    const double maxOrigin = formatter_.ToDisplayUnit(formatter_.FromMillimeters(10000.0));

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

    formatCombo_->setCurrentIndex(formatIndexFromName(currentSettings.formatName));
    orientationCombo_->setCurrentIndex(
        currentSettings.orientation == TDVecPageOrientation::Landscape ? 1 : 0);
    widthSpin_->setValue(formatter_.ToDisplayUnit(currentSettings.widthReal));
    heightSpin_->setValue(formatter_.ToDisplayUnit(currentSettings.heightReal));
    originXSpin_->setValue(formatter_.ToDisplayUnit(currentSettings.pageOriginX));
    originYSpin_->setValue(formatter_.ToDisplayUnit(currentSettings.pageOriginY));
    updateDimensionReadOnly();

    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PageSetupDialog::onFormatChanged);
    connect(orientationCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PageSetupDialog::onOrientationChanged);
}

TDVecPageSettings PageSetupDialog::pageSettings() const {
    TDVecPageSettings settings;

    switch (formatCombo_->currentIndex()) {
    case kFormatA4: settings.formatName = "A4"; break;
    case kFormatA3: settings.formatName = "A3"; break;
    case kFormatA5: settings.formatName = "A5"; break;
    case kFormatLetter: settings.formatName = "Letter"; break;
    default: settings.formatName = "Custom"; break;
    }

    settings.orientation = orientationCombo_->currentIndex() == 1
        ? TDVecPageOrientation::Landscape : TDVecPageOrientation::Portrait;
    settings.widthReal = formatter_.FromDisplayUnit(widthSpin_->value());
    settings.heightReal = formatter_.FromDisplayUnit(heightSpin_->value());
    settings.pageOriginX = formatter_.FromDisplayUnit(originXSpin_->value());
    settings.pageOriginY = formatter_.FromDisplayUnit(originYSpin_->value());
    return settings;
}

void PageSetupDialog::onFormatChanged(int index) {
    if (index == kFormatCustom) {
        updateDimensionReadOnly();
        return;
    }

    const TDVecPageOrientation orientation = orientationCombo_->currentIndex() == 1
        ? TDVecPageOrientation::Landscape : TDVecPageOrientation::Portrait;
    const TDVecPageSettings preset = pageSettingsForFormat(index, orientation);
    widthSpin_->setValue(formatter_.ToDisplayUnit(preset.widthReal));
    heightSpin_->setValue(formatter_.ToDisplayUnit(preset.heightReal));
    updateDimensionReadOnly();
}

void PageSetupDialog::onOrientationChanged(int index) {
    Q_UNUSED(index);

    if (formatCombo_->currentIndex() != kFormatCustom) {
        const TDVecPageOrientation orientation = index == 1
            ? TDVecPageOrientation::Landscape : TDVecPageOrientation::Portrait;
        const TDVecPageSettings preset = pageSettingsForFormat(
            formatCombo_->currentIndex(), orientation);
        widthSpin_->setValue(formatter_.ToDisplayUnit(preset.widthReal));
        heightSpin_->setValue(formatter_.ToDisplayUnit(preset.heightReal));
        return;
    }

    const double oldWidth = widthSpin_->value();
    const double oldHeight = heightSpin_->value();
    widthSpin_->setValue(oldHeight);
    heightSpin_->setValue(oldWidth);
}

void PageSetupDialog::updateDimensionReadOnly() {
    const bool isCustom = formatCombo_->currentIndex() == kFormatCustom;
    widthSpin_->setReadOnly(!isCustom);
    heightSpin_->setReadOnly(!isCustom);
    widthSpin_->setButtonSymbols(
        isCustom ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
    heightSpin_->setButtonSymbols(
        isCustom ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
}

QString PageSetupDialog::unitSuffix() const {
    switch (formatter_.Settings().displayUnit) {
    case TDVecDisplayUnit::Millimeter: return QStringLiteral("mm");
    case TDVecDisplayUnit::Centimeter: return QStringLiteral("cm");
    case TDVecDisplayUnit::Inch: return QStringLiteral("in");
    case TDVecDisplayUnit::RawVed: return QStringLiteral("ved");
    }
    return {};
}
