#include "vec_document_settings.h"
#include "vec_model.h"

#include <cassert>
#include <cmath>
#include <string>

namespace {

constexpr double kEpsilon = 1e-9;

bool approxEqual(double a, double b)
{
    return std::fabs(a - b) < kEpsilon;
}

// --- PageFormats ---

void testA4Portrait()
{
    const TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Portrait);
    assert(page.formatName == "A4");
    assert(approxEqual(page.widthReal, 210000.0));
    assert(approxEqual(page.heightReal, 297000.0));
    assert(page.orientation == TDVecPageOrientation::Portrait);
}

void testA4Landscape()
{
    const TDVecPageSettings page = TDVecPageFormats::A4(TDVecPageOrientation::Landscape);
    assert(page.formatName == "A4");
    assert(approxEqual(page.widthReal, 297000.0));
    assert(approxEqual(page.heightReal, 210000.0));
    assert(page.orientation == TDVecPageOrientation::Landscape);
}

void testA3Portrait()
{
    const TDVecPageSettings page = TDVecPageFormats::A3(TDVecPageOrientation::Portrait);
    assert(page.formatName == "A3");
    assert(approxEqual(page.widthReal, 297000.0));
    assert(approxEqual(page.heightReal, 420000.0));
}

void testA3Landscape()
{
    const TDVecPageSettings page = TDVecPageFormats::A3(TDVecPageOrientation::Landscape);
    assert(approxEqual(page.widthReal, 420000.0));
    assert(approxEqual(page.heightReal, 297000.0));
}

void testA5Portrait()
{
    const TDVecPageSettings page = TDVecPageFormats::A5(TDVecPageOrientation::Portrait);
    assert(page.formatName == "A5");
    assert(approxEqual(page.widthReal, 148000.0));
    assert(approxEqual(page.heightReal, 210000.0));
}

void testLetterPortrait()
{
    const TDVecPageSettings page = TDVecPageFormats::Letter(TDVecPageOrientation::Portrait);
    assert(page.formatName == "Letter");
    assert(approxEqual(page.widthReal, 215900.0));
    assert(approxEqual(page.heightReal, 279400.0));
}

void testLetterLandscape()
{
    const TDVecPageSettings page = TDVecPageFormats::Letter(TDVecPageOrientation::Landscape);
    assert(approxEqual(page.widthReal, 279400.0));
    assert(approxEqual(page.heightReal, 215900.0));
}

void testCustom()
{
    const TDVecPageSettings page = TDVecPageFormats::Custom(
        "Banner", 500000.0, 100000.0, TDVecPageOrientation::Landscape);
    assert(page.formatName == "Banner");
    assert(approxEqual(page.widthReal, 500000.0));
    assert(approxEqual(page.heightReal, 100000.0));
    assert(page.orientation == TDVecPageOrientation::Landscape);
}

// --- DocumentSettings Defaults ---

void testDocumentSettingsDefaults()
{
    TDVecDocumentSettings settings;
    assert(approxEqual(settings.unitSettings.realUnitsPerMillimeter, 1000.0));
    assert(settings.unitSettings.displayUnit == TDVecDisplayUnit::Millimeter);
    assert(approxEqual(settings.gridSettings.majorStepReal, 10000.0));
    assert(settings.gridSettings.subdivisions == 10);
    assert(settings.gridSettings.resolutionLimitPixels == 1);
    assert(settings.pageSettings.formatName == "A4");
    assert(approxEqual(settings.pageSettings.widthReal, 210000.0));
    assert(approxEqual(settings.pageSettings.heightReal, 297000.0));
    assert(settings.pageSettings.orientation == TDVecPageOrientation::Portrait);
}

// --- Model integration ---

void testModelDefaultDocumentSettings()
{
    TDVecModel model;
    const auto& ds = model.DocumentSettings();
    assert(approxEqual(ds.unitSettings.realUnitsPerMillimeter, 1000.0));
    assert(ds.unitSettings.displayUnit == TDVecDisplayUnit::Millimeter);
    assert(approxEqual(ds.gridSettings.majorStepReal, 10000.0));
    assert(ds.gridSettings.subdivisions == 10);
    assert(ds.pageSettings.formatName == "A4");
    assert(approxEqual(ds.pageSettings.widthReal, 210000.0));
    assert(approxEqual(ds.pageSettings.heightReal, 297000.0));
}

void testModelConvenienceGetters()
{
    TDVecModel model;
    assert(approxEqual(model.UnitSettings().realUnitsPerMillimeter, 1000.0));
    assert(approxEqual(model.GridSettings().majorStepReal, 10000.0));
    assert(model.PageSettings().formatName == "A4");
}

void testModelSetDocumentSettings()
{
    TDVecModel model;
    model.SetChanged(false);

    TDVecDocumentSettings newSettings;
    newSettings.unitSettings.displayUnit = TDVecDisplayUnit::Inch;
    newSettings.pageSettings = TDVecPageFormats::A3(TDVecPageOrientation::Landscape);
    newSettings.gridSettings.majorStepReal = 25400.0;
    newSettings.gridSettings.subdivisions = 8;

    model.SetDocumentSettings(newSettings);

    assert(model.UnitSettings().displayUnit == TDVecDisplayUnit::Inch);
    assert(model.PageSettings().formatName == "A3");
    assert(approxEqual(model.PageSettings().widthReal, 420000.0));
    assert(approxEqual(model.GridSettings().majorStepReal, 25400.0));
    assert(model.GridSettings().subdivisions == 8);
    assert(model.IsChanged());
}

void testModelSetDocumentSettingsRoundtrip()
{
    TDVecModel model;

    TDVecDocumentSettings settings;
    settings.unitSettings.displayUnit = TDVecDisplayUnit::Centimeter;
    settings.pageSettings = TDVecPageFormats::Letter(TDVecPageOrientation::Portrait);
    model.SetDocumentSettings(settings);

    const auto& retrieved = model.DocumentSettings();
    assert(retrieved.unitSettings.displayUnit == TDVecDisplayUnit::Centimeter);
    assert(retrieved.pageSettings.formatName == "Letter");
    assert(approxEqual(retrieved.pageSettings.widthReal, 215900.0));
}

// --- Snapshot ---

void testSnapshotContainsDocumentSettings()
{
    TDVecModel model;
    TDVecDocumentSettings settings;
    settings.unitSettings.displayUnit = TDVecDisplayUnit::Inch;
    settings.pageSettings = TDVecPageFormats::A3(TDVecPageOrientation::Portrait);
    settings.gridSettings.majorStepReal = 25400.0;
    model.SetDocumentSettings(settings);

    const TDVecModelSnapshot snapshot = model.CreateSnapshot();
    assert(snapshot.documentSettings.unitSettings.displayUnit == TDVecDisplayUnit::Inch);
    assert(snapshot.documentSettings.pageSettings.formatName == "A3");
    assert(approxEqual(snapshot.documentSettings.gridSettings.majorStepReal, 25400.0));
}

void testSnapshotRestoreDocumentSettings()
{
    TDVecModel model;

    TDVecDocumentSettings inchSettings;
    inchSettings.unitSettings.displayUnit = TDVecDisplayUnit::Inch;
    inchSettings.pageSettings = TDVecPageFormats::Letter(TDVecPageOrientation::Portrait);
    model.SetDocumentSettings(inchSettings);
    const TDVecModelSnapshot snapshot = model.CreateSnapshot();

    TDVecDocumentSettings mmSettings;
    model.SetDocumentSettings(mmSettings);
    assert(model.UnitSettings().displayUnit == TDVecDisplayUnit::Millimeter);

    model.RestoreSnapshot(snapshot, false);
    assert(model.UnitSettings().displayUnit == TDVecDisplayUnit::Inch);
    assert(model.PageSettings().formatName == "Letter");
}

} // namespace

int main()
{
    testA4Portrait();
    testA4Landscape();
    testA3Portrait();
    testA3Landscape();
    testA5Portrait();
    testLetterPortrait();
    testLetterLandscape();
    testCustom();
    testDocumentSettingsDefaults();
    testModelDefaultDocumentSettings();
    testModelConvenienceGetters();
    testModelSetDocumentSettings();
    testModelSetDocumentSettingsRoundtrip();
    testSnapshotContainsDocumentSettings();
    testSnapshotRestoreDocumentSettings();
    return 0;
}
