#include "vec_polygon.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

long pointAfterScreenSegment(TDGraphicEngine* pGE, TDMatPoint point, const std::vector<TDMatPoint>& points, long tolerance) {
    if (!pGE || points.size() < 2) {
        return -1;
    }

    const TDMatPoint mouse{
        static_cast<double>(pGE->RealToXPos(point.x)),
        static_cast<double>(pGE->RealToYPos(point.y))
    };
    for (std::size_t i = 1; i < points.size(); ++i) {
        const TDMatPoint start{
            static_cast<double>(pGE->RealToXPos(points[i - 1].x)),
            static_cast<double>(pGE->RealToYPos(points[i - 1].y))
        };
        const TDMatPoint end{
            static_cast<double>(pGE->RealToXPos(points[i].x)),
            static_cast<double>(pGE->RealToYPos(points[i].y))
        };
        if (distancePointToSegment(mouse, start, end) <= static_cast<double>(tolerance)) {
            return static_cast<long>(i - 1);
        }
    }
    return -1;
}

} // namespace

TDVecPolygon::TDVecPolygon() {
    SetType(VECOBJ_POL);
}

std::uint32_t TDVecPolygon::StreamFourCC()
{
    return VEDMakeFourCC('p', 'o', 'l', 'y');
}

std::uint32_t TDVecPolygon::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecPolygon> TDVecPolygon::ReadFrom(VEDBinaryReader& reader)
{
    auto polygon = std::make_unique<TDVecPolygon>();
    polygon->ReadObjectStateFrom(reader);
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return nullptr;
    }

    polygon->points_.clear();
    polygon->points_.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        TDMatPoint point;
        point.x = reader.ReadDouble();
        point.y = reader.ReadDouble();
        polygon->points_.push_back(point);
    }
    return reader.Ok() ? std::move(polygon) : nullptr;
}

void TDVecPolygon::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteInt32(CountPoints());
    for(const TDMatPoint& point : points_) {
        writer.WriteDouble(point.x);
        writer.WriteDouble(point.y);
    }
}

void TDVecPolygon::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    pGE->DrawPolygon(&points_, bOutLine);
}

void TDVecPolygon::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE) {
        return;
    }

    for (std::size_t i = 0; i < points_.size(); ++i) {
        TDNodeType nodeType = NODE_NORMAL;
        if (i == 0) {
            nodeType = NODE_POLY_START;
        } else if (i == points_.size() - 1) {
            nodeType = NODE_POLY_END;
        }
        pGE->DrawNode(points_[i].x, points_[i].y, nodeType, GetLockResize());
    }

    const TDMatPoint midpoint = GetMidpoint();
    pGE->DrawNode(midpoint.x, midpoint.y, NODE_MIDPOINT, GetLockPosition());
}

TDMatRect TDVecPolygon::GetFrame() const {
    return frameFromPoints(points_);
}

TDMatPoint TDVecPolygon::GetMidpoint() const {
    const TDMatRect frame = GetFrame();
    return {(frame.P1.x + frame.P3.x) / 2.0, (frame.P1.y + frame.P3.y) / 2.0};
}

std::unique_ptr<TDVecObject> TDVecPolygon::Clone() const {
    return std::make_unique<TDVecPolygon>(*this);
}

TDVecLineHitResult TDVecPolygon::HitTest(TDMatPoint point, double tolerance) const {
    const TDVecLineHitResult nodeHit = HitTestNode(point, tolerance);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }
    return hitOpenPolyline(point, points_, tolerance);
}

TDVecLineHitResult TDVecPolygon::HitTestNode(TDMatPoint point, double tolerance) const {
    if (tolerance < 0.0) {
        return {};
    }

    double bestDistance = std::numeric_limits<double>::infinity();
    for (const TDMatPoint& candidate : points_) {
        bestDistance = std::min(bestDistance, distance(point, candidate));
    }
    if (bestDistance <= tolerance) {
        return {TDVecLineHitKind::StartNode, bestDistance};
    }
    return {};
}

TDVecLineHitResult TDVecPolygon::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const TDVecLineHitResult nodeHit = HitTestNode(pGE, point, tolerancePixels);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }
    return hitOpenPolyline(pGE, point, points_, tolerancePixels);
}

TDVecLineHitResult TDVecPolygon::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    if (!pGE || tolerancePixels < 0) {
        return {};
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(point.x));
    const double mouseY = static_cast<double>(pGE->RealToYPos(point.y));
    double bestDistance = std::numeric_limits<double>::infinity();
    for (const TDMatPoint& candidate : points_) {
        bestDistance = std::min(bestDistance, std::hypot(
            static_cast<double>(pGE->RealToXPos(candidate.x)) - mouseX,
            static_cast<double>(pGE->RealToYPos(candidate.y)) - mouseY));
    }
    if (bestDistance <= static_cast<double>(tolerancePixels)) {
        return {TDVecLineHitKind::StartNode, bestDistance};
    }
    return {};
}

long TDVecPolygon::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || !GetSelect()) {
        return -1;
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(X));
    const double mouseY = static_cast<double>(pGE->RealToYPos(Y));
    for (std::size_t i = 0; i < points_.size(); ++i) {
        const double nodeDistance = std::hypot(
            static_cast<double>(pGE->RealToXPos(points_[i].x)) - mouseX,
            static_cast<double>(pGE->RealToYPos(points_[i].y)) - mouseY);
        if (nodeDistance <= static_cast<double>(Toleranz)) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

long TDVecPolygon::PointOnNode(TDMatPoint point, double tolerance) const {
    if (!GetSelect()) {
        return -1;
    }

    for (std::size_t i = 0; i < points_.size(); ++i) {
        if (distance(point, points_[i]) <= tolerance) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

long TDVecPolygon::PointAfterNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    return pointAfterScreenSegment(pGE, {X, Y}, points_, Toleranz);
}

bool TDVecPolygon::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatPoint& point : points_) {
        point.x += dx;
        point.y += dy;
    }
    return true;
}

bool TDVecPolygon::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    (void)MatCPoint;
    if (GetLockResize() || iNode < 0 || iNode >= CountPoints()) {
        return false;
    }

    points_[static_cast<std::size_t>(iNode)].x += X;
    points_[static_cast<std::size_t>(iNode)].y += Y;
    return true;
}

bool TDVecPolygon::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (GetLockResize()) {
        return false;
    }

    TransformToPoint(MatPoint);
    for (TDMatPoint& point : points_) {
        point.x *= xScale;
        point.y *= yScale;
    }
    TransformToOrigin(MatPoint);
    return true;
}

bool TDVecPolygon::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatPoint& point : points_) {
        const TDMatPoint trans{point.x - MatPoint.x, point.y - MatPoint.y};
        point.x = cosD(nAngle) * trans.x - sinD(nAngle) * trans.y + MatPoint.x;
        point.y = sinD(nAngle) * trans.x + cosD(nAngle) * trans.y + MatPoint.y;
    }
    return true;
}

void TDVecPolygon::TransformToPoint(TDMatPoint MatPoint) {
    for (TDMatPoint& point : points_) {
        point.x -= MatPoint.x;
        point.y -= MatPoint.y;
    }
}

void TDVecPolygon::TransformToOrigin(TDMatPoint MatPoint) {
    for (TDMatPoint& point : points_) {
        point.x += MatPoint.x;
        point.y += MatPoint.y;
    }
}

bool TDVecPolygon::InsertPoint(int iPoint, const TDMatPoint& point) {
    if (GetLockResize() || iPoint < 0 || iPoint > CountPoints()) {
        return false;
    }
    points_.insert(points_.begin() + iPoint, point);
    return true;
}

bool TDVecPolygon::AppendPoint(const TDMatPoint& point) {
    points_.push_back(point);
    return true;
}

bool TDVecPolygon::RemovePoint(int iPoint) {
    if (GetLockResize() || points_.size() < 3 || iPoint < 0 || iPoint >= CountPoints()) {
        return false;
    }
    points_.erase(points_.begin() + iPoint);
    return true;
}

bool TDVecPolygon::ClearPoints() {
    points_.clear();
    return true;
}

const TDMatPoint* TDVecPolygon::GetPoint(int iPoint) const {
    if (iPoint < 0 || iPoint >= CountPoints()) {
        return nullptr;
    }
    return &points_[static_cast<std::size_t>(iPoint)];
}

int TDVecPolygon::CountPoints() const {
    return static_cast<int>(points_.size());
}
