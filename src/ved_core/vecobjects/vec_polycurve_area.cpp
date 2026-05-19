#include "vec_polycurve_area.h"

#include "ved_binary_format.h"
#include "ved_binary_reader.h"
#include "ved_binary_writer.h"
#include "vec_object_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

long pointAfterConturScreenSegment(TDGraphicEngine* pGE, TDMatPoint point, const std::vector<TDMatConturPoint>& points, long tolerance) {
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

TDVecPolyCurveArea::TDVecPolyCurveArea()
    : rectangularLock_(false),
      resolution_(5),
      showControls_(false),
      showPolygon_(false) {
    SetType(VECOBJ_POLYCURVEAREA);
}

std::uint32_t TDVecPolyCurveArea::StreamFourCC()
{
    return VEDMakeFourCC('p', 'c', 'a', 'r');
}

std::uint32_t TDVecPolyCurveArea::TypeFourCC() const
{
    return StreamFourCC();
}

std::unique_ptr<TDVecPolyCurveArea> TDVecPolyCurveArea::ReadFrom(VEDBinaryReader& reader)
{
    auto area = std::make_unique<TDVecPolyCurveArea>();
    area->ReadObjectStateFrom(reader);
    area->rectangularLock_ = reader.ReadBool();
    area->resolution_ = reader.ReadUInt32();
    area->showControls_ = reader.ReadBool();
    area->showPolygon_ = reader.ReadBool();
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return nullptr;
    }

    area->conturPoints_.clear();
    area->conturPoints_.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        TDMatConturPoint point;
        point.x = reader.ReadDouble();
        point.y = reader.ReadDouble();
        point.eType = static_cast<TDConturPointType>(reader.ReadEnum());
        point.bJoint = reader.ReadBool();
        area->conturPoints_.push_back(point);
    }
    return reader.Ok() ? std::move(area) : nullptr;
}

void TDVecPolyCurveArea::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
    writer.WriteBool(rectangularLock_);
    writer.WriteUInt32(resolution_);
    writer.WriteBool(showControls_);
    writer.WriteBool(showPolygon_);
    writer.WriteInt32(CountPoints());
    for(const TDMatConturPoint& point : conturPoints_) {
        writer.WriteDouble(point.x);
        writer.WriteDouble(point.y);
        writer.WriteEnum(point.eType);
        writer.WriteBool(point.bJoint);
    }
}

void TDVecPolyCurveArea::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE || conturPoints_.empty()) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    const TDMatConturPoint* previous = &conturPoints_.front();
    for (std::size_t i = 1; i < conturPoints_.size(); ++i) {
        const TDMatConturPoint* current = &conturPoints_[i];
        if (current->eType == CPT_PRIME_LINE && previous->eType == CPT_PRIME_LINE) {
            TDMatLine params{previous->x, previous->y, current->x, current->y};
            pGE->DrawLine(&params, bOutLine);
        } else if (current->eType == CPT_PRIME_QSPLINE && i + 1 < conturPoints_.size()) {
            const TDMatConturPoint* next = &conturPoints_[i + 1];
            const TDMatPoint pointA{previous->x, previous->y};
            const TDMatPoint pointB{current->x, current->y};
            const TDMatPoint pointC{next->x, next->y};
            TDMatPointsArray qCurve;
            qCurve.push_back(pointA);
            for (unsigned int step = 0; step < resolution_; ++step) {
                TDMatPoint qPoint;
                qPoint.x = (((pointA.x - 2 * pointB.x + pointC.x) * step * step) / (resolution_ * resolution_)) +
                           (((2 * pointB.x - 2 * pointA.x) * step) / resolution_) + pointA.x;
                qPoint.y = (((pointA.y - 2 * pointB.y + pointC.y) * step * step) / (resolution_ * resolution_)) +
                           (((2 * pointB.y - 2 * pointA.y) * step) / resolution_) + pointA.y;
                qCurve.push_back(qPoint);
            }
            qCurve.push_back(pointC);
            pGE->DrawPolygon(&qCurve, bOutLine);

            if (GetShowPolygon()) {
                TDMatPointsArray construct{pointA, pointB, pointC};
                pGE->DrawConstructPolygon(&construct);
            }
        }
        previous = current;
    }
}

void TDVecPolyCurveArea::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE || conturPoints_.empty()) {
        return;
    }

    const bool resizeLock = GetLockResize();
    if (!rectangularLock_) {
        for (const TDMatConturPoint& point : conturPoints_) {
            if (point.eType != CPT_PRIME_QSPLINE) {
                pGE->DrawNode(point.x, point.y, NODE_NORMAL, resizeLock);
            } else if (GetShowControls()) {
                pGE->DrawNode(point.x, point.y, NODE_CONTROL, resizeLock);
            }
        }
    } else {
        const TDMatRect frame = GetFrame();
        const TDMatRect middle = frameMidEdges(frame);
        pGE->DrawNode(frame.P1.x, frame.P1.y, NODE_NORMAL, resizeLock);
        pGE->DrawNode(frame.P2.x, frame.P2.y, NODE_NORMAL, resizeLock);
        pGE->DrawNode(frame.P3.x, frame.P3.y, NODE_NORMAL, resizeLock);
        pGE->DrawNode(frame.P4.x, frame.P4.y, NODE_NORMAL, resizeLock);
        pGE->DrawNode(middle.P1.x, middle.P1.y, NODE_NORMAL, resizeLock);
        pGE->DrawNode(middle.P2.x, middle.P2.y, NODE_NORMAL, resizeLock);
        pGE->DrawNode(middle.P3.x, middle.P3.y, NODE_NORMAL, resizeLock);
        pGE->DrawNode(middle.P4.x, middle.P4.y, NODE_NORMAL, resizeLock);
    }
}

TDMatRect TDVecPolyCurveArea::GetFrame() const {
    return frameFromConturPoints(conturPoints_);
}

TDMatPoint TDVecPolyCurveArea::GetMidpoint() const {
    const TDMatRect frame = GetFrame();
    return {(frame.P1.x + frame.P3.x) / 2.0, (frame.P1.y + frame.P3.y) / 2.0};
}

std::unique_ptr<TDVecObject> TDVecPolyCurveArea::Clone() const {
    return std::make_unique<TDVecPolyCurveArea>(*this);
}

TDVecLineHitResult TDVecPolyCurveArea::HitTest(TDMatPoint point, double tolerance) const {
    std::vector<TDMatPoint> plainPoints;
    plainPoints.reserve(conturPoints_.size());
    for (const TDMatConturPoint& conturPoint : conturPoints_) {
        if (conturPoint.eType == CPT_PRIME_LINE) {
            plainPoints.push_back({conturPoint.x, conturPoint.y});
        }
    }
    return hitClosedPolyline(point, plainPoints, tolerance);
}

TDVecLineHitResult TDVecPolyCurveArea::HitTestNode(TDMatPoint point, double tolerance) const {
    const long node = PointOnNode(point, tolerance);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

TDVecLineHitResult TDVecPolyCurveArea::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    if (!pGE) {
        return {};
    }

    std::vector<TDMatPoint> plainPoints;
    plainPoints.reserve(conturPoints_.size());
    for (const TDMatConturPoint& conturPoint : conturPoints_) {
        if (conturPoint.eType == CPT_PRIME_LINE) {
            plainPoints.push_back({conturPoint.x, conturPoint.y});
        }
    }
    return hitClosedPolyline(pGE, point, plainPoints, tolerancePixels);
}

TDVecLineHitResult TDVecPolyCurveArea::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const long node = PointOnNode(pGE, point.x, point.y, tolerancePixels);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

long TDVecPolyCurveArea::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || !GetSelect()) {
        return -1;
    }
    if (rectangularLock_) {
        return PointOnFrameNode(pGE, X, Y, Toleranz);
    }

    const double mouseX = static_cast<double>(pGE->RealToXPos(X));
    const double mouseY = static_cast<double>(pGE->RealToYPos(Y));
    for (std::size_t i = 0; i < conturPoints_.size(); ++i) {
        const TDMatConturPoint& point = conturPoints_[i];
        const double nodeDistance = std::hypot(
            static_cast<double>(pGE->RealToXPos(point.x)) - mouseX,
            static_cast<double>(pGE->RealToYPos(point.y)) - mouseY);
        if (nodeDistance <= static_cast<double>(Toleranz)) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

long TDVecPolyCurveArea::PointOnNode(TDMatPoint point, double tolerance) const {
    if (!GetSelect()) {
        return -1;
    }
    if (rectangularLock_) {
        return PointOnFrameNode(point, tolerance);
    }

    for (std::size_t i = 0; i < conturPoints_.size(); ++i) {
        if (distance(point, {conturPoints_[i].x, conturPoints_[i].y}) <= tolerance) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

long TDVecPolyCurveArea::PointAfterNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (rectangularLock_) {
        return -1;
    }
    return pointAfterConturScreenSegment(pGE, {X, Y}, conturPoints_, Toleranz);
}

bool TDVecPolyCurveArea::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatConturPoint& point : conturPoints_) {
        point.x += dx;
        point.y += dy;
    }
    return true;
}

bool TDVecPolyCurveArea::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    if (GetLockResize()) {
        return false;
    }
    if (rectangularLock_) {
        return MoveFrameNode(iNode, X, Y, MatCPoint);
    }
    if (iNode < 0 || iNode >= CountPoints()) {
        return false;
    }

    auto movePoint = [this, X, Y](long index) {
        if (index >= 0 && index < CountPoints()) {
            conturPoints_[static_cast<std::size_t>(index)].x += X;
            conturPoints_[static_cast<std::size_t>(index)].y += Y;
        }
    };
    movePoint(iNode);
    if (iNode == 0) {
        movePoint(CountPoints() - 1);
    }
    if (iNode == CountPoints() - 1) {
        movePoint(0);
    }
    return true;
}

bool TDVecPolyCurveArea::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (GetLockResize()) {
        return false;
    }

    TransformToPoint(MatPoint);
    for (TDMatConturPoint& point : conturPoints_) {
        point.x *= xScale;
        point.y *= yScale;
    }
    TransformToOrigin(MatPoint);
    return true;
}

bool TDVecPolyCurveArea::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatConturPoint& point : conturPoints_) {
        const TDMatPoint trans{point.x - MatPoint.x, point.y - MatPoint.y};
        point.x = cosD(nAngle) * trans.x - sinD(nAngle) * trans.y + MatPoint.x;
        point.y = sinD(nAngle) * trans.x + cosD(nAngle) * trans.y + MatPoint.y;
    }
    return true;
}

void TDVecPolyCurveArea::TransformToPoint(TDMatPoint MatPoint) {
    for (TDMatConturPoint& point : conturPoints_) {
        point.x -= MatPoint.x;
        point.y -= MatPoint.y;
    }
}

void TDVecPolyCurveArea::TransformToOrigin(TDMatPoint MatPoint) {
    for (TDMatConturPoint& point : conturPoints_) {
        point.x += MatPoint.x;
        point.y += MatPoint.y;
    }
}

void TDVecPolyCurveArea::SetRectangularLock(bool bValue) {
    rectangularLock_ = bValue;
}

bool TDVecPolyCurveArea::GetRectangularLock() const {
    return rectangularLock_;
}

void TDVecPolyCurveArea::SetShowControls(bool bShowControls) {
    showControls_ = bShowControls;
}

bool TDVecPolyCurveArea::GetShowControls() const {
    return showControls_;
}

void TDVecPolyCurveArea::SetShowPolygon(bool bShowPolygon) {
    showPolygon_ = bShowPolygon;
}

bool TDVecPolyCurveArea::GetShowPolygon() const {
    return showPolygon_;
}

void TDVecPolyCurveArea::SetResolution(unsigned int nResolution) {
    resolution_ = nResolution != 0 ? nResolution : 5;
}

unsigned int TDVecPolyCurveArea::GetResolution() const {
    return resolution_;
}

bool TDVecPolyCurveArea::ChangePointType(int iPoint) {
    if (GetLockResize() || iPoint <= 0 || iPoint >= CountPoints() - 1) {
        return false;
    }

    TDMatConturPoint& point = conturPoints_[static_cast<std::size_t>(iPoint)];
    if (point.eType == CPT_PRIME_QSPLINE) {
        point.eType = CPT_PRIME_LINE;
        return true;
    }

    const TDMatConturPoint& prev = conturPoints_[static_cast<std::size_t>(iPoint - 1)];
    const TDMatConturPoint& next = conturPoints_[static_cast<std::size_t>(iPoint + 1)];
    if (prev.eType != CPT_PRIME_QSPLINE && next.eType != CPT_PRIME_QSPLINE) {
        point.eType = CPT_PRIME_QSPLINE;
        return true;
    }
    return false;
}

bool TDVecPolyCurveArea::InsertPoint(int iPoint, const TDMatConturPoint& point) {
    if (GetLockResize() || iPoint < 0 || iPoint > CountPoints()) {
        return false;
    }
    conturPoints_.insert(conturPoints_.begin() + iPoint, point);
    return true;
}

bool TDVecPolyCurveArea::AppendPoint(const TDMatConturPoint& point) {
    conturPoints_.push_back(point);
    return true;
}

bool TDVecPolyCurveArea::RemovePoint(int iPoint) {
    if (GetLockResize() || conturPoints_.size() < 4 || iPoint < 0 || iPoint >= CountPoints()) {
        return false;
    }
    conturPoints_.erase(conturPoints_.begin() + iPoint);
    return true;
}

bool TDVecPolyCurveArea::ClearPoints() {
    conturPoints_.clear();
    return true;
}

const TDMatConturPoint* TDVecPolyCurveArea::GetPoint(int iPoint) const {
    if (iPoint < 0 || iPoint >= CountPoints()) {
        return nullptr;
    }
    return &conturPoints_[static_cast<std::size_t>(iPoint)];
}

int TDVecPolyCurveArea::CountPoints() const {
    return static_cast<int>(conturPoints_.size());
}
