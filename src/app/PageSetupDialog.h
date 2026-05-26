#pragma once

#include "vec_document_settings.h"
#include "vec_units.h"

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;

class PageSetupDialog : public QDialog {
public:
    PageSetupDialog(const TDVecPageSettings& currentSettings,
                    const TDVecUnitSettings& unitSettings,
                    QWidget* parent = nullptr);

    TDVecPageSettings pageSettings() const;

private:
    void onFormatChanged(int index);
    void onOrientationChanged(int index);
    void updateDimensionReadOnly();
    QString unitSuffix() const;

    TDVecUnitFormatter formatter_;
    QComboBox* formatCombo_;
    QComboBox* orientationCombo_;
    QDoubleSpinBox* widthSpin_;
    QDoubleSpinBox* heightSpin_;
    QDoubleSpinBox* originXSpin_;
    QDoubleSpinBox* originYSpin_;
};
