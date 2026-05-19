#include "vec_polygon_area.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

long pointAfterClosedScreenSegment(TDGraphicEngine* pGE, TDMatPoint point, const std::vector<TDMatPoint>& points, long tolerance) {
    if (!pGE || points.size() < 2) {
        return -1;
    }

    std::vector<TDMatPoint> closedPoints = points;
    closedPoints.push_back(points.front());
    const TDMatPoint mouse{
        static_cast<double>(pGE->RealToXPos(point.x)),
        static_cast<double>(pGE->RealToYPos(point.y))
    };
    for (std::size_t i = 1; i < closedPoints.size(); ++i) {
        const TDMatPoint start{
            static_cast<double>(pGE->RealToXPos(closedPoints[i - 1].x)),
            static_cast<double>(pGE->RealToYPos(closedPoints[i - 1].y))
        };
        const TDMatPoint end{
            static_cast<double>(pGE->RealToXPos(closedPoints[i].x)),
            static_cast<double>(pGE->RealToYPos(closedPoints[i].y))
        };
        if (distancePointToSegment(mouse, start, end) <= static_cast<double>(tolerance)) {
            return static_cast<long>(i - 1);
        }
    }
    return -1;
}

} // namespace

TDVecPolygonArea::TDVecPolygonArea()
    : rectangularLock_(false) {
    SetType(VECOBJ_POLAREA);
}

std::uint32_t TDVecPolygonArea::StreamFourCC()
{
    return VEDMakeFourCC('p', 'o', 'l', 'a');
}

std::uint32_t TDVecPolygonArea::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecPolygonArea> TDVecPolygonArea::ReadFrom(VEDBinaryReader& reader)
{
    auto polygonArea = std::make_unique<TDVecPolygonArea>();
    polygonArea->ReadObjectStateFrom(reader);
    polygonArea->rectangularLock_ = reader.ReadBool();
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return nullptr;
    }

    polygonArea->points_.clear();
    polygonArea->points_.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        TDMatPoint point;
        point.x = reader.ReadDouble();
        point.y = reader.ReadDouble();
        polygonArea->points_.push_back(point);
    }
    return reader.Ok() ? std::move(polygonArea) : nullptr;
}

void TDVecPolygonArea::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteBool(rectangularLock_);
    writer.WriteInt32(CountPoints());
    for(const TDMatPoint& point : points_) {
        writer.WriteDouble(point.x);
        writer.WriteDouble(point.y);
    }
}

void TDVecPolygonArea::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    if (points_.empty()) {
        return;
    }

    TDMatPointsArray tmpPoints = points_;
    tmpPoints.push_back(points_.front());
    pGE->DrawPolygon(&tmpPoints, bOutLine);
}

void TDVecPolygonArea::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE) {
        return;
    }

    for (const TDMatPoint& point : points_) {
        pGE->DrawNode(point.x, point.y, NODE_NORMAL, GetLockResize());
    }

    const TDMatPoint midpoint = GetMidpoint();
    pGE->DrawNode(midpoint.x, midpoint.y, NODE_MIDPOINT, GetLockPosition());
}

TDMatRect TDVecPolygonArea::GetFrame() const {
    return frameFromPoints(points_);
}

TDMatPoint TDVecPolygonArea::GetMidpoint() const {
    const TDMatRect frame = GetFrame();
    return {(frame.P1.x + frame.P3.x) / 2.0, (frame.P1.y + frame.P3.y) / 2.0};
}

std::unique_ptr<TDVecObject> TDVecPolygonArea::Clone() const {
    return std::make_unique<TDVecPolygonArea>(*this);
}

TDVecLineHitResult TDVecPolygonArea::HitTest(TDMatPoint point, double tolerance) const {
    const TDVecLineHitResult nodeHit = HitTestNode(point, tolerance);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }
    return hitClosedPolyline(point, points_, tolerance);
}

TDVecLineHitResult TDVecPolygonArea::HitTestNode(TDMatPoint point, double tolerance) const {
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

TDVecLineHitResult TDVecPolygonArea::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const TDVecLineHitResult nodeHit = HitTestNode(pGE, point, tolerancePixels);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }
    return hitClosedPolyline(pGE, point, points_, tolerancePixels);
}

TDVecLineHitResult TDVecPolygonArea::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
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

long TDVecPolygonArea::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || !GetSelect()) {
        return -1;
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(X));
    const double mouseY = static_cast<double>(pGE->RealToYPos(Y));
    for (std::size_t i = 0; i < points_.size(); ++i) {
        const TDMatPoint& point = points_[i];
        const double nodeDistance = std::hypot(
            static_cast<double>(pGE->RealToXPos(point.x)) - mouseX,
            static_cast<double>(pGE->RealToYPos(point.y)) - mouseY);
        if (nodeDistance <= static_cast<double>(Toleranz)) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

long TDVecPolygonArea::PointOnNode(TDMatPoint point, double tolerance) const {
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

long TDVecPolygonArea::PointAfterNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    return pointAfterClosedScreenSegment(pGE, {X, Y}, points_, Toleranz);
}

bool TDVecPolygonArea::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatPoint& point : points_) {
        point.x += dx;
        point.y += dy;
    }
    return true;
}

bool TDVecPolygonArea::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    (void)MatCPoint;
    if (GetLockResize() || iNode < 0 || iNode >= CountPoints()) {
        return false;
    }

    if (rectangularLock_ && points_.size() == 4) {
        const std::size_t node = static_cast<std::size_t>(iNode);
        const std::size_t prev = (node + 3) % 4;
        const std::size_t next = (node + 1) % 4;
        const std::size_t opposite = (node + 2) % 4;
        TDMatPoint moved{points_[node].x + X, points_[node].y + Y};
        points_[node] = moved;
        points_[prev] = projectPointOnLine(points_[opposite], points_[prev], moved);
        points_[next] = projectPointOnLine(points_[opposite], points_[next], moved);
        return true;
    }

    points_[static_cast<std::size_t>(iNode)].x += X;
    points_[static_cast<std::size_t>(iNode)].y += Y;
    return true;
}

bool TDVecPolygonArea::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
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

bool TDVecPolygonArea::Rotate(TDMatPoint MatPoint, double nAngle) {
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

void TDVecPolygonArea::TransformToPoint(TDMatPoint MatPoint) {
    for (TDMatPoint& point : points_) {
        point.x -= MatPoint.x;
        point.y -= MatPoint.y;
    }
}

void TDVecPolygonArea::TransformToOrigin(TDMatPoint MatPoint) {
    for (TDMatPoint& point : points_) {
        point.x += MatPoint.x;
        point.y += MatPoint.y;
    }
}

void TDVecPolygonArea::SetRectangularLock(bool bValue) {
    rectangularLock_ = bValue;
}

bool TDVecPolygonArea::GetRectangularLock() const {
    return rectangularLock_;
}

bool TDVecPolygonArea::InsertPoint(int iPoint, const TDMatPoint& point) {
    if (GetLockResize() || iPoint < 0 || iPoint > CountPoints()) {
        return false;
    }
    points_.insert(points_.begin() + iPoint, point);
    return true;
}

bool TDVecPolygonArea::AppendPoint(const TDMatPoint& point) {
    points_.push_back(point);
    return true;
}

bool TDVecPolygonArea::RemovePoint(int iPoint) {
    if (GetLockResize() || points_.size() < 4 || iPoint < 0 || iPoint >= CountPoints()) {
        return false;
    }
    points_.erase(points_.begin() + iPoint);
    return true;
}

bool TDVecPolygonArea::ClearPoints() {
    points_.clear();
    return true;
}

const TDMatPoint* TDVecPolygonArea::GetPoint(int iPoint) const {
    if (iPoint < 0 || iPoint >= CountPoints()) {
        return nullptr;
    }
    return &points_[static_cast<std::size_t>(iPoint)];
}

int TDVecPolygonArea::CountPoints() const {
    return static_cast<int>(points_.size());
}
