#include "ved_binary_writer.h"
#include "vec_bezier_curve.h"
#include "vec_bspline.h"
#include "vec_ellipse.h"
#include "vec_font.h"
#include "vec_font_manager.h"
#include "vec_group.h"
#include "ved_font_resolution.h"
#include "ved_model_io.h"
#include "vec_line.h"
#include "vec_model.h"
#include "vec_polycurve.h"
#include "vec_polycurve_area.h"
#include "vec_polygon.h"
#include "vec_polygon_area.h"
#include "vec_text.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

class TestFontProvider : public IVecFontProvider {
public:
    explicit TestFontProvider(std::vector<std::string> fontIds)
        : fontIds_(std::move(fontIds)) {
    }

    std::vector<TDVecFontDescriptor> AvailableFonts() const override {
        std::vector<TDVecFontDescriptor> fonts;
        for (const std::string& fontId : fontIds_) {
            fonts.push_back({fontId, fontId, fontId == kVEDWpsDefaultFontId, fontId.rfind("TT:", 0) == 0});
        }
        return fonts;
    }

    std::unique_ptr<TDVecFont> LoadFont(const std::string& fontId) override {
        for (const std::string& availableFontId : fontIds_) {
            if (availableFontId == fontId) {
                auto font = std::make_unique<TDVecFont>();
                font->SetFontName(fontId.c_str());
                font->SetHeight(1000.0);
                font->SetAscent(800.0);
                font->SetSpacingGlyphWidth(400.0);
                return font;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::string> fontIds_;
};

constexpr double kEpsilon = 0.000001;
constexpr std::uint32_t kTestModelMetaChunk = VEDMakeFourCC('M', 'E', 'T', 'A');
constexpr std::uint32_t kTestModelObjectsChunk = VEDMakeFourCC('O', 'B', 'J', 'S');

bool nearlyEqual(double left, double right)
{
    return std::fabs(left - right) < kEpsilon;
}

void assertPointEqual(TDMatPoint actual, TDMatPoint expected)
{
    assert(nearlyEqual(actual.x, expected.x));
    assert(nearlyEqual(actual.y, expected.y));
}

void writeValidHeaderAndMetadata(VEDBinaryWriter& writer)
{
    writer.WriteFourCC(VEDMakeFourCC('V', 'E', 'D', 'M'));
    writer.WriteUInt16(1U);
    writer.WriteUInt16(0U);
    writer.WriteUInt16(2U);

    VEDBinaryWriter metadata;
    metadata.WriteDouble(0.0);
    metadata.WriteDouble(0.0);
    metadata.WriteDouble(100.0);
    metadata.WriteDouble(200.0);
    metadata.WriteUInt32(0x00000000U);
    writer.WriteFourCC(kTestModelMetaChunk);
    writer.WriteUInt32(static_cast<std::uint32_t>(metadata.Size()));
    writer.WriteBytes(metadata.Bytes());
}

void writeEmptyObjectsChunk(VEDBinaryWriter& writer)
{
    VEDBinaryWriter objects;
    objects.WriteUInt32(0U);
    writer.WriteFourCC(kTestModelObjectsChunk);
    writer.WriteUInt32(static_cast<std::uint32_t>(objects.Size()));
    writer.WriteBytes(objects.Bytes());
}

const TDVecPolyCurve* requirePolyCurve(const TDVecModel& model, int index)
{
    const auto* curve = dynamic_cast<const TDVecPolyCurve*>(model.GetObject(index));
    assert(curve);
    return curve;
}

const TDVecLine* requireLine(const TDVecModel& model, int index)
{
    const auto* line = dynamic_cast<const TDVecLine*>(model.GetObject(index));
    assert(line);
    return line;
}

const TDVecEllipse* requireEllipse(const TDVecModel& model, int index)
{
    const auto* ellipse = dynamic_cast<const TDVecEllipse*>(model.GetObject(index));
    assert(ellipse);
    return ellipse;
}

const TDVecPolygon* requirePolygon(const TDVecModel& model, int index)
{
    const auto* polygon = dynamic_cast<const TDVecPolygon*>(model.GetObject(index));
    assert(polygon);
    return polygon;
}

const TDVecPolygonArea* requirePolygonArea(const TDVecModel& model, int index)
{
    const auto* polygonArea = dynamic_cast<const TDVecPolygonArea*>(model.GetObject(index));
    assert(polygonArea);
    return polygonArea;
}

const TDVecBezierCurve* requireBezier(const TDVecModel& model, int index)
{
    const auto* bezier = dynamic_cast<const TDVecBezierCurve*>(model.GetObject(index));
    assert(bezier);
    return bezier;
}

const TDVecBSPLine* requireBSPLine(const TDVecModel& model, int index)
{
    const auto* bspline = dynamic_cast<const TDVecBSPLine*>(model.GetObject(index));
    assert(bspline);
    return bspline;
}

const TDVecPolyCurveArea* requirePolyCurveArea(const TDVecModel& model, int index)
{
    const auto* area = dynamic_cast<const TDVecPolyCurveArea*>(model.GetObject(index));
    assert(area);
    return area;
}

const TDVecGroup* requireGroup(const TDVecModel& model, int index)
{
    const auto* group = dynamic_cast<const TDVecGroup*>(model.GetObject(index));
    assert(group);
    return group;
}

} // namespace

int main()
{
    {
        TDVecModel model;
        model.SetTopLeftArea({-10.5, -20.25});
        model.SetBottomRightArea({300.75, 400.125});
        model.SetDefaultColor(0x00aabbccU);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());
        assert(!writeResult.bytes.empty());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes);
        assert(readResult.Ok());
        assert(readResult.model);
        assert(readResult.model->CountObjects() == 0);
        assertPointEqual(readResult.model->GetTopLeftArea(), {-10.5, -20.25});
        assertPointEqual(readResult.model->GetBottomRightArea(), {300.75, 400.125});
        assert(readResult.model->GetDefaultColor() == 0x00aabbccU);
        assert(!readResult.viewState.present);
    }

    {
        TDVecModel model;
        VEDDocumentViewState viewState;
        viewState.present = true;
        viewState.zoom = 2.5;
        viewState.center = {12345.5, 67890.25};
        viewState.showGrid = true;
        viewState.gridLock = true;
        viewState.showRulers = true;

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model, viewState);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes);
        assert(readResult.Ok());
        assert(readResult.viewState.present);
        assert(nearlyEqual(readResult.viewState.zoom, 2.5));
        assertPointEqual(readResult.viewState.center, {12345.5, 67890.25});
        assert(readResult.viewState.showGrid);
        assert(readResult.viewState.gridLock);
        assert(readResult.viewState.showRulers);
    }

    {
        VEDBinaryWriter writer;
        writeValidHeaderAndMetadata(writer);
        writer.WriteFourCC(VEDMakeFourCC('V', 'I', 'E', 'W'));
        writer.WriteUInt32(1U);
        writer.WriteUInt8(1U);
        writeEmptyObjectsChunk(writer);

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writer.Bytes());
        assert(readResult.Ok());
        assert(!readResult.viewState.present);
    }

    {
        TDVecModel model;
        auto* curve = new TDVecPolyCurve();
        curve->SetResolution(9);
        curve->SetShowControls(false);
        curve->SetShowPolygon(true);
        curve->AppendPoint({1.0, 2.0, CPT_PRIME_LINE, true});
        curve->AppendPoint({3.0, 4.0, CPT_PRIME_QSPLINE, false});
        model.AppendObject(curve);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes);
        assert(readResult.Ok());
        assert(readResult.model);
        assert(readResult.model->CountObjects() == 1);

        const TDVecPolyCurve* readCurve = requirePolyCurve(*readResult.model, 0);
        assert(readCurve->GetParentModel() == readResult.model.get());
        assert(readCurve->GetResolution() == 9U);
        assert(!readCurve->GetShowControls());
        assert(readCurve->GetShowPolygon());
        assert(readCurve->CountPoints() == 2);
        const TDMatConturPoint* point = readCurve->GetPoint(1);
        assert(point);
        assert(nearlyEqual(point->x, 3.0));
        assert(nearlyEqual(point->y, 4.0));
        assert(point->eType == CPT_PRIME_QSPLINE);
        assert(!point->bJoint);
    }

    {
        TDVecModel model;
        model.AppendObject(new TDVecLine(1.0, 2.0, 3.0, 4.0));
        model.AppendObject(new TDVecEllipse(5.0, 6.0, 7.0, 8.0, 35.0));
        auto* polygon = new TDVecPolygon();
        polygon->AppendPoint({10.0, 11.0});
        polygon->AppendPoint({12.0, 13.0});
        polygon->AppendPoint({14.0, 15.0});
        model.AppendObject(polygon);
        auto* polygonArea = new TDVecPolygonArea();
        polygonArea->SetRectangularLock(true);
        polygonArea->AppendPoint({20.0, 21.0});
        polygonArea->AppendPoint({22.0, 23.0});
        polygonArea->AppendPoint({24.0, 25.0});
        model.AppendObject(polygonArea);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes);
        assert(readResult.Ok());
        assert(readResult.model);
        assert(readResult.model->CountObjects() == 4);

        const TDMatLine readLine = requireLine(*readResult.model, 0)->GetLine();
        assert(nearlyEqual(readLine.x1, 1.0));
        assert(nearlyEqual(readLine.y1, 2.0));
        assert(nearlyEqual(readLine.x2, 3.0));
        assert(nearlyEqual(readLine.y2, 4.0));

        const TDMatEllipse readEllipse = requireEllipse(*readResult.model, 1)->GetEllipse();
        assert(nearlyEqual(readEllipse.xCenter, 5.0));
        assert(nearlyEqual(readEllipse.yCenter, 6.0));
        assert(nearlyEqual(readEllipse.xRadius, 7.0));
        assert(nearlyEqual(readEllipse.yRadius, 8.0));
        assert(nearlyEqual(readEllipse.nAngle, 35.0));

        const TDVecPolygon* readPolygon = requirePolygon(*readResult.model, 2);
        assert(readPolygon->CountPoints() == 3);
        const TDMatPoint* polygonPoint = readPolygon->GetPoint(2);
        assert(polygonPoint);
        assert(nearlyEqual(polygonPoint->x, 14.0));
        assert(nearlyEqual(polygonPoint->y, 15.0));

        const TDVecPolygonArea* readPolygonArea = requirePolygonArea(*readResult.model, 3);
        assert(readPolygonArea->GetRectangularLock());
        assert(readPolygonArea->CountPoints() == 3);
        const TDMatPoint* polygonAreaPoint = readPolygonArea->GetPoint(1);
        assert(polygonAreaPoint);
        assert(nearlyEqual(polygonAreaPoint->x, 22.0));
        assert(nearlyEqual(polygonAreaPoint->y, 23.0));
    }

    {
        TDVecModel model;
        auto* bezier = new TDVecBezierCurve();
        bezier->SetResolution(17);
        bezier->SetShowControls(true);
        bezier->SetShowPolygon(true);
        bezier->AppendPoint({1.0, 1.5});
        bezier->AppendPoint({2.0, 2.5});
        bezier->AppendPoint({3.0, 3.5});
        model.AppendObject(bezier);

        auto* bspline = new TDVecBSPLine();
        bspline->SetDegree(3);
        bspline->SetResolution(23);
        bspline->SetShowControls(true);
        bspline->SetShowPolygon(false);
        bspline->AppendPoint({10.0, 10.5});
        bspline->AppendPoint({11.0, 11.5});
        bspline->AppendPoint({12.0, 12.5});
        bspline->AppendPoint({13.0, 13.5});
        model.AppendObject(bspline);

        auto* polyCurveArea = new TDVecPolyCurveArea();
        polyCurveArea->SetRectangularLock(true);
        polyCurveArea->SetResolution(6);
        polyCurveArea->SetShowControls(true);
        polyCurveArea->SetShowPolygon(false);
        polyCurveArea->AppendPoint({20.0, 20.5, CPT_PRIME_LINE, true});
        polyCurveArea->AppendPoint({21.0, 21.5, CPT_PRIME_QSPLINE, false});
        polyCurveArea->AppendPoint({22.0, 22.5, CPT_PRIME_LINE, true});
        model.AppendObject(polyCurveArea);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes);
        assert(readResult.Ok());
        assert(readResult.model);
        assert(readResult.model->CountObjects() == 3);

        const TDVecBezierCurve* readBezier = requireBezier(*readResult.model, 0);
        assert(readBezier->GetResolution() == 17U);
        assert(readBezier->GetShowControls());
        assert(readBezier->GetShowPolygon());
        assert(readBezier->CountPoints() == 3);
        const TDMatPoint* bezierPoint = readBezier->GetPoint(2);
        assert(bezierPoint);
        assert(nearlyEqual(bezierPoint->x, 3.0));
        assert(nearlyEqual(bezierPoint->y, 3.5));

        const TDVecBSPLine* readBSPLine = requireBSPLine(*readResult.model, 1);
        assert(readBSPLine->GetDegree() == 3U);
        assert(readBSPLine->GetResolution() == 23U);
        assert(readBSPLine->GetShowControls());
        assert(!readBSPLine->GetShowPolygon());
        assert(readBSPLine->CountPoints() == 4);
        const TDMatPoint* bsplinePoint = readBSPLine->GetPoint(3);
        assert(bsplinePoint);
        assert(nearlyEqual(bsplinePoint->x, 13.0));
        assert(nearlyEqual(bsplinePoint->y, 13.5));

        const TDVecPolyCurveArea* readArea = requirePolyCurveArea(*readResult.model, 2);
        assert(readArea->GetRectangularLock());
        assert(readArea->GetResolution() == 6U);
        assert(readArea->GetShowControls());
        assert(!readArea->GetShowPolygon());
        assert(readArea->CountPoints() == 3);
        const TDMatConturPoint* areaPoint = readArea->GetPoint(1);
        assert(areaPoint);
        assert(nearlyEqual(areaPoint->x, 21.0));
        assert(nearlyEqual(areaPoint->y, 21.5));
        assert(areaPoint->eType == CPT_PRIME_QSPLINE);
        assert(!areaPoint->bJoint);
    }

    {
        TDVecModel model;
        auto* group = new TDVecGroup();
        group->AppendInGroup(new TDVecLine(1.0, 2.0, 3.0, 4.0));
        auto* polygon = new TDVecPolygon();
        polygon->AppendPoint({10.0, 11.0});
        polygon->AppendPoint({12.0, 13.0});
        group->AppendInGroup(polygon);
        group->SetResolveLock(true);
        group->FirstInitialize();
        model.AppendObject(group);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes);
        assert(readResult.Ok());
        assert(readResult.model);
        assert(readResult.model->CountObjects() == 1);

        const TDVecGroup* readGroup = requireGroup(*readResult.model, 0);
        assert(readGroup->GetParentModel() == readResult.model.get());
        assert(readGroup->GetResolveLock());
        assert(readGroup->CountObjectsInGroup() == 2);

        const std::unique_ptr<TDVecObject> readLineObject = readGroup->GetCopyFromGroup(0);
        const auto* readLine = dynamic_cast<const TDVecLine*>(readLineObject.get());
        assert(readLine);
        const TDMatLine line = readLine->GetLine();
        assert(nearlyEqual(line.x1, 1.0));
        assert(nearlyEqual(line.y1, 2.0));
        assert(nearlyEqual(line.x2, 3.0));
        assert(nearlyEqual(line.y2, 4.0));
    }

    {
        TestFontProvider provider({kVEDArialUnicodeMSFontId, kVEDWpsDefaultFontId});
        TDFontManager fontManager;
        fontManager.RegisterProvider(&provider);
        assert(fontManager.SetDefaultFontId(kVEDWpsDefaultFontId));

        TDVecModel model;
        auto* text = new TDVecText();
        text->SetFontName("TT:Missing Font");
        text->SetText("Fallback");
        model.AppendObject(text);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes, fontManager);
        assert(readResult.Ok());
        assert(readResult.fontResolution.Ok());
        assert(readResult.fontResolution.warnings.size() == 1U);
        assert(readResult.fontResolution.warnings[0].kind == VEDFontFallbackKind::RequestedMissingUsedArialUnicodeMS);
        const auto* readText = dynamic_cast<const TDVecText*>(readResult.model->GetObject(0));
        assert(readText);
        assert(std::string(readText->GetFontName()) == kVEDArialUnicodeMSFontId);
        assert(readText->GetVecFontPointer());
    }

    {
        TestFontProvider provider({kVEDWpsDefaultFontId});
        TDFontManager fontManager;
        fontManager.RegisterProvider(&provider);
        assert(fontManager.SetDefaultFontId(kVEDWpsDefaultFontId));

        TDVecModel model;
        auto* group = new TDVecGroup();
        auto* text = new TDVecText();
        text->SetFontName("TT:Missing Nested Font");
        text->SetText("Nested");
        group->AppendInGroup(text);
        group->FirstInitialize();
        model.AppendObject(group);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes, fontManager);
        assert(readResult.Ok());
        assert(readResult.fontResolution.Ok());
        assert(readResult.fontResolution.warnings.size() == 1U);
        assert(readResult.fontResolution.warnings[0].kind == VEDFontFallbackKind::RequestedMissingUsedWpsDefault);
        const TDVecGroup* readGroup = requireGroup(*readResult.model, 0);
        const std::unique_ptr<TDVecObject> readTextObject = readGroup->GetCopyFromGroup(0);
        const auto* readText = dynamic_cast<const TDVecText*>(readTextObject.get());
        assert(readText);
        assert(std::string(readText->GetFontName()) == kVEDWpsDefaultFontId);
        assert(readText->GetVecFontPointer());
    }

    {
        TestFontProvider provider({kVEDSegoeUIFontId, kVEDWpsDefaultFontId});
        TDFontManager fontManager;
        fontManager.RegisterProvider(&provider);
        assert(fontManager.SetDefaultFontId(kVEDWpsDefaultFontId));

        TDVecModel model;
        auto* text = new TDVecText();
        text->SetFontName("TT:Missing Windows Font");
        text->SetText("Segoe");
        model.AppendObject(text);

        const VEDModelWriteResult writeResult = SaveVecModelToBytes(model);
        assert(writeResult.Ok());

        const VEDModelReadResult readResult = LoadVecModelFromBytes(writeResult.bytes, fontManager);
        assert(readResult.Ok());
        assert(readResult.fontResolution.Ok());
        assert(readResult.fontResolution.warnings.size() == 1U);
        assert(readResult.fontResolution.warnings[0].kind == VEDFontFallbackKind::RequestedMissingUsedSegoeUI);
        const auto* readText = dynamic_cast<const TDVecText*>(readResult.model->GetObject(0));
        assert(readText);
        assert(std::string(readText->GetFontName()) == kVEDSegoeUIFontId);
        assert(readText->GetVecFontPointer());
    }

    {
        VEDBinaryWriter writer;
        writer.WriteFourCC(VEDMakeFourCC('B', 'A', 'D', '!'));
        writer.WriteUInt16(1U);
        writer.WriteUInt16(0U);
        writer.WriteUInt16(1U);
        const VEDModelReadResult result = LoadVecModelFromBytes(writer.Bytes());
        assert(!result.Ok());
        assert(result.error == VEDBinaryError::InvalidSignature);
    }

    {
        VEDBinaryWriter writer;
        writer.WriteFourCC(VEDMakeFourCC('V', 'E', 'D', 'M'));
        writer.WriteUInt16(2U);
        writer.WriteUInt16(0U);
        writer.WriteUInt16(0U);
        const VEDModelReadResult result = LoadVecModelFromBytes(writer.Bytes());
        assert(!result.Ok());
        assert(result.error == VEDBinaryError::UnsupportedVersion);
    }

    {
        VEDBinaryWriter writer;
        writer.WriteFourCC(VEDMakeFourCC('V', 'E', 'D', 'M'));
        writer.WriteUInt16(1U);
        writer.WriteUInt16(0U);
        writer.WriteUInt16(1U);
        const VEDModelReadResult result = LoadVecModelFromBytes(writer.Bytes());
        assert(!result.Ok());
        assert(result.error == VEDBinaryError::UnsupportedVersion);
    }

    {
        const std::byte truncated[] = {std::byte{'V'}, std::byte{'E'}};
        const VEDModelReadResult result = LoadVecModelFromBytes(std::span<const std::byte>(truncated));
        assert(!result.Ok());
        assert(result.error == VEDBinaryError::EndOfBuffer);
    }

    {
        VEDBinaryWriter writer;
        writeValidHeaderAndMetadata(writer);

        VEDBinaryWriter objects;
        objects.WriteUInt32(1U);
        objects.WriteFourCC(VEDMakeFourCC('?', '?', '?', '?'));
        objects.WriteUInt32(0U);
        writer.WriteFourCC(kTestModelObjectsChunk);
        writer.WriteUInt32(static_cast<std::uint32_t>(objects.Size()));
        writer.WriteBytes(objects.Bytes());

        const VEDModelReadResult result = LoadVecModelFromBytes(writer.Bytes());
        assert(!result.Ok());
        assert(result.error == VEDBinaryError::UnknownObjectType);
    }

    return 0;
}
