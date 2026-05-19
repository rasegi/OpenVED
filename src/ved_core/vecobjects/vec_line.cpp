#include "vec_line.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

TDVecLine::TDVecLine()
    : TDVecLine(0.0, 0.0, 0.0, 0.0) {
}

TDVecLine::TDVecLine(double x1, double y1, double x2, double y2)
    : x1_(x1),
      y1_(y1),
      x2_(x2),
      y2_(y2) {
    SetType(VECOBJ_LIN);
}

std::uint32_t TDVecLine::StreamFourCC()
{
    return VEDMakeFourCC('l', 'i', 'n', 'e');
}

std::uint32_t TDVecLine::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecLine> TDVecLine::ReadFrom(VEDBinaryReader& reader)
{
    auto line = std::make_unique<TDVecLine>();
    line->ReadObjectStateFrom(reader);
    line->x1_ = reader.ReadDouble();
    line->y1_ = reader.ReadDouble();
    line->x2_ = reader.ReadDouble();
    line->y2_ = reader.ReadDouble();
    return reader.Ok() ? std::move(line) : nullptr;
}

void TDVecLine::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteDouble(x1_);
    writer.WriteDouble(y1_);
    writer.WriteDouble(x2_);
    writer.WriteDouble(y2_);
}

void TDVecLine::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    TDMatLine params{x1_, y1_, x2_, y2_};
    pGE->DrawLine(&params, bOutLine);
}

void TDVecLine::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE) {
        return;
    }

    pGE->DrawNode(x1_, y1_, NODE_NORMAL, GetLockResize());
    pGE->DrawNode(x2_, y2_, NODE_NORMAL, GetLockResize());
    const TDMatPoint midpoint = GetMidpoint();
    pGE->DrawNode(midpoint.x, midpoint.y, NODE_MIDPOINT, GetLockPosition());
}

TDMatRect TDVecLine::GetFrame() const {
    return makeFrameFromBounds(
        std::min(x1_, x2_),
        std::min(y1_, y2_),
        std::max(x1_, x2_),
        std::max(y1_, y2_));
}

TDMatPoint TDVecLine::GetMidpoint() const {
    return {(x1_ + x2_) / 2.0, (y1_ + y2_) / 2.0};
}

std::unique_ptr<TDVecObject> TDVecLine::Clone() const {
    return std::make_unique<TDVecLine>(*this);
}

TDVecLineHitResult TDVecLine::HitTest(TDMatPoint point, double tolerance) const {
    const TDVecLineHitResult nodeHit = HitTestNode(point, tolerance);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }
    return HitTestLine(point, tolerance);
}

TDVecLineHitResult TDVecLine::HitTestLine(TDMatPoint point, double tolerance) const {
    if (tolerance < 0.0) {
        return {};
    }

    const TDMatPoint start{x1_, y1_};
    const TDMatPoint end{x2_, y2_};
    const double hitDistance = distancePointToSegment(point, start, end);
    if (hitDistance <= tolerance) {
        return {TDVecLineHitKind::Body, hitDistance};
    }
    return {};
}

TDVecLineHitResult TDVecLine::HitTestNode(TDMatPoint point, double tolerance) const {
    if (tolerance < 0.0) {
        return {};
    }

    const TDMatPoint start{x1_, y1_};
    const TDMatPoint end{x2_, y2_};
    const TDMatPoint midpoint = GetMidpoint();
    const std::array candidates{
        TDVecLineHitResult{TDVecLineHitKind::StartNode, distance(point, start)},
        TDVecLineHitResult{TDVecLineHitKind::EndNode, distance(point, end)},
        TDVecLineHitResult{TDVecLineHitKind::MidpointNode, distance(point, midpoint)}
    };

    TDVecLineHitResult best;
    best.distance = std::numeric_limits<double>::infinity();
    for (const TDVecLineHitResult& candidate : candidates) {
        if (candidate.distance <= tolerance && candidate.distance < best.distance) {
            best = candidate;
        }
    }
    if (best.IsHit()) {
        return best;
    }
    return {};
}

TDVecLineHitResult TDVecLine::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const TDVecLineHitResult nodeHit = HitTestNode(pGE, point, tolerancePixels);
    if (nodeHit.IsHit()) {
        return nodeHit;
    }
    return HitTestLine(pGE, point, tolerancePixels);
}

TDVecLineHitResult TDVecLine::HitTestLine(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    if (!pGE || tolerancePixels < 0) {
        return {};
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(point.x));
    const double mouseY = static_cast<double>(pGE->RealToYPos(point.y));
    const double startX = static_cast<double>(pGE->RealToXPos(x1_));
    const double startY = static_cast<double>(pGE->RealToYPos(y1_));
    const double endX = static_cast<double>(pGE->RealToXPos(x2_));
    const double endY = static_cast<double>(pGE->RealToYPos(y2_));

    const double hitDistance = distancePointToSegment(mouseX, mouseY, startX, startY, endX, endY);
    if (hitDistance <= static_cast<double>(tolerancePixels)) {
        return {TDVecLineHitKind::Body, hitDistance};
    }
    return {};
}

TDVecLineHitResult TDVecLine::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    if (!pGE || tolerancePixels < 0) {
        return {};
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(point.x));
    const double mouseY = static_cast<double>(pGE->RealToYPos(point.y));
    const TDMatPoint midpoint = GetMidpoint();
    const std::array candidates{
        TDVecLineHitResult{
            TDVecLineHitKind::StartNode,
            std::hypot(static_cast<double>(pGE->RealToXPos(x1_)) - mouseX, static_cast<double>(pGE->RealToYPos(y1_)) - mouseY)
        },
        TDVecLineHitResult{
            TDVecLineHitKind::EndNode,
            std::hypot(static_cast<double>(pGE->RealToXPos(x2_)) - mouseX, static_cast<double>(pGE->RealToYPos(y2_)) - mouseY)
        },
        TDVecLineHitResult{
            TDVecLineHitKind::MidpointNode,
            std::hypot(static_cast<double>(pGE->RealToXPos(midpoint.x)) - mouseX, static_cast<double>(pGE->RealToYPos(midpoint.y)) - mouseY)
        }
    };

    TDVecLineHitResult best;
    best.distance = std::numeric_limits<double>::infinity();
    for (const TDVecLineHitResult& candidate : candidates) {
        if (candidate.distance <= static_cast<double>(tolerancePixels) && candidate.distance < best.distance) {
            best = candidate;
        }
    }
    if (best.IsHit()) {
        return best;
    }
    return {};
}

bool TDVecLine::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    x1_ += dx;
    y1_ += dy;
    x2_ += dx;
    y2_ += dy;
    return true;
}

long TDVecLine::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || !GetSelect()) {
        return -1;
    }

    const POINT mouse{pGE->RealToXPos(X), pGE->RealToYPos(Y)};
    const POINT p1{pGE->RealToXPos(x1_), pGE->RealToYPos(y1_)};
    const POINT p2{pGE->RealToXPos(x2_), pGE->RealToYPos(y2_)};
    const double distP1 = std::hypot(static_cast<double>(p1.x - mouse.x), static_cast<double>(p1.y - mouse.y));
    const double distP2 = std::hypot(static_cast<double>(p2.x - mouse.x), static_cast<double>(p2.y - mouse.y));

    if (distP1 <= static_cast<double>(Toleranz)) {
        return 0;
    }
    if (distP2 <= static_cast<double>(Toleranz)) {
        return 1;
    }
    return -1;
}

long TDVecLine::PointOnNode(TDMatPoint point, double tolerance) const {
    if (!GetSelect()) {
        return -1;
    }

    if (distance(point, {x1_, y1_}) <= tolerance) {
        return 0;
    }
    if (distance(point, {x2_, y2_}) <= tolerance) {
        return 1;
    }
    return -1;
}

bool TDVecLine::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    (void)MatCPoint;
    if (GetLockResize()) {
        return false;
    }

    if (iNode == 0) {
        x1_ += X;
        y1_ += Y;
        return true;
    }
    if (iNode == 1) {
        x2_ += X;
        y2_ += Y;
        return true;
    }
    return false;
}

bool TDVecLine::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (GetLockResize()) {
        return false;
    }

    x1_ = (xScale * (x1_ - MatPoint.x)) + MatPoint.x;
    y1_ = (yScale * (y1_ - MatPoint.y)) + MatPoint.y;
    x2_ = (xScale * (x2_ - MatPoint.x)) + MatPoint.x;
    y2_ = (yScale * (y2_ - MatPoint.y)) + MatPoint.y;
    return true;
}

bool TDVecLine::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    TransformToPoint(MatPoint);
    const TDMatPoint point1{x1_, y1_};
    const TDMatPoint point2{x2_, y2_};
    x1_ = cosD(nAngle) * point1.x - sinD(nAngle) * point1.y;
    y1_ = sinD(nAngle) * point1.x + cosD(nAngle) * point1.y;
    x2_ = cosD(nAngle) * point2.x - sinD(nAngle) * point2.y;
    y2_ = sinD(nAngle) * point2.x + cosD(nAngle) * point2.y;
    TransformToOrigin(MatPoint);
    return true;
}

void TDVecLine::TransformToPoint(TDMatPoint MatPoint) {
    x1_ -= MatPoint.x;
    y1_ -= MatPoint.y;
    x2_ -= MatPoint.x;
    y2_ -= MatPoint.y;
}

void TDVecLine::TransformToOrigin(TDMatPoint MatPoint) {
    x1_ += MatPoint.x;
    y1_ += MatPoint.y;
    x2_ += MatPoint.x;
    y2_ += MatPoint.y;
}

bool TDVecLine::MoveNode(TDVecLineHitKind nodeKind, TDMatPoint point) {
    if (GetLockResize() &&
        (nodeKind == TDVecLineHitKind::StartNode || nodeKind == TDVecLineHitKind::EndNode)) {
        return false;
    }

    switch (nodeKind) {
    case TDVecLineHitKind::StartNode:
        x1_ = point.x;
        y1_ = point.y;
        return true;
    case TDVecLineHitKind::EndNode:
        x2_ = point.x;
        y2_ = point.y;
        return true;
    case TDVecLineHitKind::MidpointNode: {
        const TDMatPoint midpoint = GetMidpoint();
        return MoveBy(point.x - midpoint.x, point.y - midpoint.y);
    }
    default:
        return false;
    }
}

void TDVecLine::SetLine(double x1, double y1, double x2, double y2) {
    x1_ = x1;
    y1_ = y1;
    x2_ = x2;
    y2_ = y2;
}

void TDVecLine::SetLine(const TDMatLine* pMatLine) {
    if (!pMatLine) {
        return;
    }

    SetLine(pMatLine->x1, pMatLine->y1, pMatLine->x2, pMatLine->y2);
}

TDMatLine TDVecLine::GetLine() const {
    return {x1_, y1_, x2_, y2_};
}
