#include "ved_object_io.h"

#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_bezier_curve.h"
#include "vec_bspline.h"
#include "vec_group.h"
#include "vec_object.h"
#include "vec_ellipse.h"
#include "vec_line.h"
#include "vec_polycurve.h"
#include "vec_polycurve_area.h"
#include "vec_polygon.h"
#include "vec_polygon_area.h"
#include "vec_text.h"

namespace {

bool isSupportedCoreObjectType(std::uint32_t typeFourCC)
{
    return typeFourCC == TDVecPolyCurve::StreamFourCC() ||
           typeFourCC == TDVecLine::StreamFourCC() ||
           typeFourCC == TDVecEllipse::StreamFourCC() ||
           typeFourCC == TDVecPolygon::StreamFourCC() ||
           typeFourCC == TDVecPolygonArea::StreamFourCC() ||
           typeFourCC == TDVecBezierCurve::StreamFourCC() ||
           typeFourCC == TDVecBSPLine::StreamFourCC() ||
           typeFourCC == TDVecGroup::StreamFourCC() ||
           typeFourCC == TDVecPolyCurveArea::StreamFourCC() ||
           typeFourCC == TDVecText::StreamFourCC() ||
           typeFourCC == TDVecFrameText::StreamFourCC();
}

std::unique_ptr<TDVecObject> readSupportedCoreObject(std::uint32_t typeFourCC, VEDBinaryReader& reader)
{
    if(typeFourCC == TDVecPolyCurve::StreamFourCC()) {
        return TDVecPolyCurve::ReadFrom(reader);
    }
    if(typeFourCC == TDVecLine::StreamFourCC()) {
        return TDVecLine::ReadFrom(reader);
    }
    if(typeFourCC == TDVecEllipse::StreamFourCC()) {
        return TDVecEllipse::ReadFrom(reader);
    }
    if(typeFourCC == TDVecPolygon::StreamFourCC()) {
        return TDVecPolygon::ReadFrom(reader);
    }
    if(typeFourCC == TDVecPolygonArea::StreamFourCC()) {
        return TDVecPolygonArea::ReadFrom(reader);
    }
    if(typeFourCC == TDVecBezierCurve::StreamFourCC()) {
        return TDVecBezierCurve::ReadFrom(reader);
    }
    if(typeFourCC == TDVecBSPLine::StreamFourCC()) {
        return TDVecBSPLine::ReadFrom(reader);
    }
    if(typeFourCC == TDVecGroup::StreamFourCC()) {
        return TDVecGroup::ReadFrom(reader);
    }
    if(typeFourCC == TDVecPolyCurveArea::StreamFourCC()) {
        return TDVecPolyCurveArea::ReadFrom(reader);
    }
    if(typeFourCC == TDVecText::StreamFourCC()) {
        return TDVecText::ReadFrom(reader);
    }
    if(typeFourCC == TDVecFrameText::StreamFourCC()) {
        return TDVecFrameText::ReadFrom(reader);
    }
    reader.Fail(VEDBinaryError::UnknownObjectType);
    return nullptr;
}

} // namespace

bool VEDObjectWriteResult::Ok() const noexcept
{
    return error == VEDBinaryError::None;
}

bool VEDObjectReadResult::Ok() const noexcept
{
    return error == VEDBinaryError::None && object != nullptr;
}

VEDObjectWriteResult SaveVecObjectPayload(const TDVecObject& object)
{
    VEDObjectWriteResult result;
    result.typeFourCC = object.TypeFourCC();
    if(!isSupportedCoreObjectType(result.typeFourCC)) {
        result.error = VEDBinaryError::UnsupportedObjectType;
        return result;
    }

    VEDBinaryWriter writer;
    object.WriteTo(writer);
    result.payload = writer.Buffer();
    return result;
}

VEDObjectReadResult LoadVecObjectPayload(std::uint32_t typeFourCC, std::span<const std::byte> payload)
{
    VEDBinaryReader reader(payload);
    std::unique_ptr<TDVecObject> object = readSupportedCoreObject(typeFourCC, reader);
    if(!reader.Ok()) {
        return {nullptr, reader.Error()};
    }
    if(!object) {
        return {nullptr, VEDBinaryError::UnknownObjectType};
    }
    if(!reader.End()) {
        return {nullptr, VEDBinaryError::InvalidArgument};
    }
    return {std::move(object), VEDBinaryError::None};
}
