#pragma once

#include "vec_document_settings.h"
#include "vec_units.h"

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;

class DocumentSetupDialog : public QDialog {
public:
    DocumentSetupDialog(const TDVecDocumentSettings& currentSettings,
                        QWidget* parent = nullptr);

    TDVecDocumentSettings documentSettings() const;

private:
    void onUnitChanged(int index);
    void onFormatChanged(int index);
    void onOrientationChanged(int index);
    void updateDimensionReadOnly();
    void updateSpinBoxUnit();
    TDVecDisplayUnit selectedUnit() const;
    double toDisplay(double realValue) const;
    double fromDisplay(double displayValue) const;
    QString unitSuffix() const;

    TDVecUnitSettings unitSettings_;
    QComboBox* unitCombo_;
    QComboBox* formatCombo_;
    QComboBox* orientationCombo_;
    QDoubleSpinBox* widthSpin_;
    QDoubleSpinBox* heightSpin_;
    QDoubleSpinBox* originXSpin_;
    QDoubleSpinBox* originYSpin_;
};
