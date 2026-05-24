#include "vec_polycurve.h"

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

TDVecPolyCurve::TDVecPolyCurve()
    : resolution_(5),
      showControls_(true),
      showPolygon_(true) {
    SetType(VECOBJ_POLYCURVE);
}

std::unique_ptr<TDVecPolyCurve> TDVecPolyCurve::ReadFrom(VEDBinaryReader& reader)
{
    auto curve = std::make_unique<TDVecPolyCurve>();
    curve->ReadObjectStateFrom(reader);
    curve->resolution_ = reader.ReadUInt32();
    curve->showControls_ = reader.ReadBool();
    curve->showPolygon_ = reader.ReadBool();
    const std::int32_t count = reader.ReadInt32();
    if(count < 0) {
        reader.Fail(VEDBinaryError::InvalidArgument);
        return nullptr;
    }

    curve->conturPoints_.clear();
    curve->conturPoints_.reserve(static_cast<std::size_t>(count));
    for(std::int32_t i = 0; i < count; ++i) {
        TDMatConturPoint point;
        point.x = reader.ReadDouble();
        point.y = reader.ReadDouble();
        point.eType = static_cast<TDConturPointType>(reader.ReadEnum());
        point.bJoint = reader.ReadBool();
        curve->conturPoints_.push_back(point);
    }
    return reader.Ok() ? std::move(curve) : nullptr;
}

void TDVecPolyCurve::EnsureDrawCacheComputed() const {
    if (!drawCacheDirty_) {
        return;
    }
    drawCache_ = polyCurveDrawPoints(conturPoints_, resolution_);
    drawCacheDirty_ = false;
}

void TDVecPolyCurve::MarkDrawCacheDirty() {
    drawCacheDirty_ = true;
}

void TDVecPolyCurve::Draw(TDGraphicEngine* pGE, bool bOutLine) const {
    if (!pGE || conturPoints_.empty()) {
        return;
    }

    TDVecObject::Draw(pGE, bOutLine);
    EnsureDrawCacheComputed();
    pGE->DrawPolygon(&drawCache_, bOutLine);

    if (GetShowPolygon()) {
        const TDMatConturPoint* previous = &conturPoints_.front();
        for (std::size_t i = 1; i < conturPoints_.size(); ++i) {
            const TDMatConturPoint* current = &conturPoints_[i];
            if (current->eType == CPT_PRIME_QSPLINE && i + 1 < conturPoints_.size()) {
                const TDMatConturPoint* next = &conturPoints_[i + 1];
                TDMatPointsArray construct{{previous->x, previous->y}, {current->x, current->y}, {next->x, next->y}};
                pGE->DrawConstructPolygon(&construct);
            }
            previous = current;
        }
    }
}

std::uint32_t TDVecPolyCurve::StreamFourCC()
{
    return VEDMakeFourCC('p', 'l', 'y', 'c');
}

std::uint32_t TDVecPolyCurve::TypeFourCC() const
{
    return StreamFourCC();
}

void TDVecPolyCurve::WriteTo(VEDBinaryWriter& writer) const
{
    TDVecObject::WriteTo(writer);
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

void TDVecPolyCurve::DrawNodes(TDGraphicEngine* pGE) const {
    if (!pGE || conturPoints_.empty()) {
        return;
    }

    for (const TDMatConturPoint& point : conturPoints_) {
        if (point.eType != CPT_PRIME_QSPLINE) {
            pGE->DrawNode(point.x, point.y, NODE_NORMAL, GetLockResize());
        } else if (GetShowControls()) {
            pGE->DrawNode(point.x, point.y, NODE_CONTROL, GetLockResize());
        }
    }

    const TDMatPoint midpoint = GetMidpoint();
    pGE->DrawNode(midpoint.x, midpoint.y, NODE_MIDPOINT, GetLockPosition());
}

TDMatRect TDVecPolyCurve::GetFrame() const {
    return frameFromConturPoints(conturPoints_);
}

TDMatPoint TDVecPolyCurve::GetMidpoint() const {
    const TDMatRect frame = GetFrame();
    return {(frame.P1.x + frame.P3.x) / 2.0, (frame.P1.y + frame.P3.y) / 2.0};
}

std::unique_ptr<TDVecObject> TDVecPolyCurve::Clone() const {
    return std::make_unique<TDVecPolyCurve>(*this);
}

TDVecLineHitResult TDVecPolyCurve::HitTest(TDMatPoint point, double tolerance) const {
    EnsureDrawCacheComputed();
    return hitOpenPolyline(point, drawCache_, tolerance);
}

TDVecLineHitResult TDVecPolyCurve::HitTestNode(TDMatPoint point, double tolerance) const {
    const long node = PointOnNode(point, tolerance);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

TDVecLineHitResult TDVecPolyCurve::HitTest(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    EnsureDrawCacheComputed();
    return hitOpenPolyline(pGE, point, drawCache_, tolerancePixels);
}

TDVecLineHitResult TDVecPolyCurve::HitTestNode(TDGraphicEngine* pGE, TDMatPoint point, long tolerancePixels) const {
    const long node = PointOnNode(pGE, point.x, point.y, tolerancePixels);
    if (node >= 0) {
        return {TDVecLineHitKind::StartNode, 0.0};
    }
    return {};
}

long TDVecPolyCurve::PointOnNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    if (!pGE || !GetSelect()) {
        return -1;
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

long TDVecPolyCurve::PointOnNode(TDMatPoint point, double tolerance) const {
    if (!GetSelect()) {
        return -1;
    }

    for (std::size_t i = 0; i < conturPoints_.size(); ++i) {
        if (distance(point, {conturPoints_[i].x, conturPoints_[i].y}) <= tolerance) {
            return static_cast<long>(i);
        }
    }
    return -1;
}

long TDVecPolyCurve::PointAfterNode(TDGraphicEngine* pGE, double X, double Y, long Toleranz) const {
    return pointAfterConturScreenSegment(pGE, {X, Y}, conturPoints_, Toleranz);
}

bool TDVecPolyCurve::MoveBy(double dx, double dy) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatConturPoint& point : conturPoints_) {
        point.x += dx;
        point.y += dy;
    }
    MarkDrawCacheDirty();
    return true;
}

bool TDVecPolyCurve::MoveNode(long iNode, double X, double Y, TDMatPoint MatCPoint) {
    (void)MatCPoint;
    if (GetLockResize() || iNode < 0 || iNode >= CountPoints()) {
        return false;
    }

    conturPoints_[static_cast<std::size_t>(iNode)].x += X;
    conturPoints_[static_cast<std::size_t>(iNode)].y += Y;
    MarkDrawCacheDirty();
    return true;
}

bool TDVecPolyCurve::ToScale(TDMatPoint MatPoint, double xScale, double yScale) {
    if (GetLockResize()) {
        return false;
    }

    TransformToPoint(MatPoint);
    for (TDMatConturPoint& point : conturPoints_) {
        point.x *= xScale;
        point.y *= yScale;
    }
    TransformToOrigin(MatPoint);
    MarkDrawCacheDirty();
    return true;
}

bool TDVecPolyCurve::Rotate(TDMatPoint MatPoint, double nAngle) {
    if (GetLockPosition()) {
        return false;
    }

    for (TDMatConturPoint& point : conturPoints_) {
        const TDMatPoint trans{point.x - MatPoint.x, point.y - MatPoint.y};
        point.x = cosD(nAngle) * trans.x - sinD(nAngle) * trans.y + MatPoint.x;
        point.y = sinD(nAngle) * trans.x + cosD(nAngle) * trans.y + MatPoint.y;
    }
    MarkDrawCacheDirty();
    return true;
}

void TDVecPolyCurve::TransformToPoint(TDMatPoint MatPoint) {
    for (TDMatConturPoint& point : conturPoints_) {
        point.x -= MatPoint.x;
        point.y -= MatPoint.y;
    }
    MarkDrawCacheDirty();
}

void TDVecPolyCurve::TransformToOrigin(TDMatPoint MatPoint) {
    for (TDMatConturPoint& point : conturPoints_) {
        point.x += MatPoint.x;
        point.y += MatPoint.y;
    }
    MarkDrawCacheDirty();
}

void TDVecPolyCurve::SetShowControls(bool bShowControls) {
    showControls_ = bShowControls;
}

bool TDVecPolyCurve::GetShowControls() const {
    return showControls_;
}

void TDVecPolyCurve::SetShowPolygon(bool bShowPolygon) {
    showPolygon_ = bShowPolygon;
}

bool TDVecPolyCurve::GetShowPolygon() const {
    return showPolygon_;
}

void TDVecPolyCurve::SetResolution(unsigned int nResolution) {
    resolution_ = nResolution != 0 ? nResolution : 5;
    MarkDrawCacheDirty();
}

unsigned int TDVecPolyCurve::GetResolution() const {
    return resolution_;
}

void TDVecPolyCurve::DrawPolyCurve(TDGraphicEngine* pGE, bool bOutLine, unsigned int nResolution) const {
    const unsigned int oldResolution = resolution_;
    const_cast<TDVecPolyCurve*>(this)->SetResolution(nResolution);
    Draw(pGE, bOutLine);
    const_cast<TDVecPolyCurve*>(this)->SetResolution(oldResolution);
}

bool TDVecPolyCurve::ChangePointType(int iPoint) {
    if (GetLockResize() || iPoint <= 0 || iPoint >= CountPoints() - 1) {
        return false;
    }

    TDMatConturPoint& point = conturPoints_[static_cast<std::size_t>(iPoint)];
    if (point.eType == CPT_PRIME_QSPLINE) {
        point.eType = CPT_PRIME_LINE;
        MarkDrawCacheDirty();
        return true;
    }

    const TDMatConturPoint& prev = conturPoints_[static_cast<std::size_t>(iPoint - 1)];
    const TDMatConturPoint& next = conturPoints_[static_cast<std::size_t>(iPoint + 1)];
    if (prev.eType != CPT_PRIME_QSPLINE && next.eType != CPT_PRIME_QSPLINE) {
        point.eType = CPT_PRIME_QSPLINE;
        MarkDrawCacheDirty();
        return true;
    }
    return false;
}

bool TDVecPolyCurve::InsertPoint(int iPoint, const TDMatConturPoint& point) {
    if (GetLockResize() || iPoint < 0 || iPoint > CountPoints()) {
        return false;
    }

    if (point.eType == CPT_PRIME_QSPLINE) {
        if (iPoint <= 0 || iPoint >= CountPoints()) {
            return false;
        }
        const TDMatConturPoint& prev = conturPoints_[static_cast<std::size_t>(iPoint - 1)];
        const TDMatConturPoint& next = conturPoints_[static_cast<std::size_t>(iPoint)];
        if (prev.eType != CPT_PRIME_LINE || next.eType != CPT_PRIME_LINE) {
            return false;
        }
    }

    conturPoints_.insert(conturPoints_.begin() + iPoint, point);
    MarkDrawCacheDirty();
    return true;
}

bool TDVecPolyCurve::AppendPoint(const TDMatConturPoint& point) {
    conturPoints_.push_back(point);
    MarkDrawCacheDirty();
    return true;
}

bool TDVecPolyCurve::RemovePoint(int iPoint) {
    if (GetLockResize() || conturPoints_.size() < 3 || iPoint < 0 || iPoint >= CountPoints()) {
        return false;
    }

    const TDMatConturPoint& current = conturPoints_[static_cast<std::size_t>(iPoint)];
    if (current.eType == CPT_PRIME_LINE) {
        if (iPoint > 0 && iPoint < CountPoints() - 1) {
            const TDMatConturPoint& prev = conturPoints_[static_cast<std::size_t>(iPoint - 1)];
            const TDMatConturPoint& next = conturPoints_[static_cast<std::size_t>(iPoint + 1)];
            if (prev.eType == CPT_PRIME_QSPLINE && next.eType == CPT_PRIME_QSPLINE) {
                return false;
            }
        } else if (iPoint == 0 && CountPoints() > 1 && conturPoints_[1].eType == CPT_PRIME_QSPLINE) {
            return false;
        } else if (iPoint == CountPoints() - 1 &&
                   CountPoints() > 1 &&
                   conturPoints_[static_cast<std::size_t>(iPoint - 1)].eType == CPT_PRIME_QSPLINE) {
            return false;
        }
    }

    conturPoints_.erase(conturPoints_.begin() + iPoint);
    MarkDrawCacheDirty();
    return true;
}

bool TDVecPolyCurve::ClearPoints() {
    conturPoints_.clear();
    drawCache_.clear();
    drawCacheDirty_ = true;
    return true;
}

const TDMatConturPoint* TDVecPolyCurve::GetPoint(int iPoint) const {
    if (iPoint < 0 || iPoint >= CountPoints()) {
        return nullptr;
    }
    return &conturPoints_[static_cast<std::size_t>(iPoint)];
}

TDMatConturPoint* TDVecPolyCurve::GetPointFree(int iPoint) {
    if (iPoint < 0 || iPoint >= CountPoints()) {
        return nullptr;
    }
    return &conturPoints_[static_cast<std::size_t>(iPoint)];
}

int TDVecPolyCurve::CountPoints() const {
    return static_cast<int>(conturPoints_.size());
}
